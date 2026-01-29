/*
 * fenac_encoder.c
 *
 * Created on: Jan 29, 2026
 * Author: User
 */

#include "fenac_encoder.h"

// Mikrosaniye bekleme fonksiyonu (Timer veya basit döngü kullanılabilir)
// Basitlik adına yaklaşık bir delay döngüsü:
void Delay_us(uint32_t us) {
    // SystemCoreClock 84MHz varsayımıyla kaba bir döngü.
    // Kendi sistem hızınıza göre kalibre etmeniz gerekebilir veya TIM kullanabilirsiniz.
    uint32_t count = us * (SystemCoreClock / 1000000) / 4;
    while(count--);
}

// Enkoder Yapısını Başlatma
void Fenac_Init(FenacEncoder_t *enc, SPI_HandleTypeDef *hspi_handle) {
	enc->hspi = hspi_handle;
	enc->raw_value = 0;
	enc->angle_degrees = 0.0f;
	enc->zero_offset = 0;
	enc->test_counter = 0;
}

// Ana Okuma Fonksiyonu
// fenac_encoder.c İÇİNDEKİ Fenac_Read_SSI Fonksiyonu

void Fenac_Read_SSI(FenacEncoder_t *enc) {

#ifdef SPI_LOOPBACK_TEST_MODE
    // --- GELİŞMİŞ SİMÜLASYON MODU (Senaryolu) ---
    // Senaryo: 0->90 (10sn) | Bekle (10sn) | 90->0 (10sn) | Tekrarla

    static uint32_t start_tick = 0;
    static uint8_t  sim_state = 0; // 0: İleri, 1: Bekle, 2: Geri

    // 18-bit Çözünürlük (262,144 adım)
    // 90 Derece = 262144 / 4 = 65536 adım
    const uint32_t MAX_POS_90 = 65536;
    const uint32_t DURATION_MS = 10000; // 10 Saniye

    // İlk girişte zamanı başlat
    if (start_tick == 0) start_tick = HAL_GetTick();

    uint32_t current_tick = HAL_GetTick();
    uint32_t elapsed = current_tick - start_tick;

    switch (sim_state)
    {
        case 0: // --- AŞAMA 1: 0'dan 90'a Yavaş Dönüş (10 sn) ---
            if (elapsed <= DURATION_MS) {
                // Zaman oranına göre pozisyon hesapla (Linear Interpolation)
                // Formül: (GeçenSüre * Hedef) / ToplamSüre
                enc->test_counter = (elapsed * MAX_POS_90) / DURATION_MS;
            } else {
                // Süre bitti, tam 90'a sabitle ve sonraki aşamaya geç
                enc->test_counter = MAX_POS_90;
                sim_state = 1;
                start_tick = current_tick; // Zamanı sıfırla
            }
            break;

        case 1: // --- AŞAMA 2: 90 Derecede Bekle (10 sn) ---
            enc->test_counter = MAX_POS_90; // Sabit tut

            if (elapsed >= DURATION_MS) {
                sim_state = 2; // Geri dönüş aşamasına geç
                start_tick = current_tick;
            }
            break;

        case 2: // --- AŞAMA 3: 90'dan 0'a Geri Dönüş (10 sn) ---
            if (elapsed <= DURATION_MS) {
                // Geri doğru hesaplama: Hedef - ((Geçen * Hedef) / Süre)
                uint32_t decrement = (elapsed * MAX_POS_90) / DURATION_MS;
                enc->test_counter = MAX_POS_90 - decrement;
            } else {
                // Süre bitti, tam 0'a sabitle
                enc->test_counter = 0;
                sim_state = 0; // En başa (Aşama 1'e) dön
                start_tick = current_tick;
            }
            break;
    }

    // --- VERİ GÖNDERME KISMI (Loopback için Paketleme) ---
    // test_counter değişkenini 32-bit SPI paketine bölüyoruz
    uint16_t tx_data[2];
    uint16_t rx_data[2];

    tx_data[0] = (enc->test_counter >> 16) & 0xFFFF; // High Word
    tx_data[1] = (enc->test_counter) & 0xFFFF;       // Low Word

    HAL_SPI_TransmitReceive(enc->hspi, (uint8_t*)&tx_data[0], (uint8_t*)&rx_data[0], 1, 100);
    HAL_SPI_TransmitReceive(enc->hspi, (uint8_t*)&tx_data[1], (uint8_t*)&rx_data[1], 1, 100);

    // Gelen veriyi birleştir
    uint32_t full_data = ((uint32_t)rx_data[0] << 16) | rx_data[1];
    enc->raw_value = full_data & ENCODER_MASK;

#else
    // --- GERÇEK DONANIM MODU (BURASI AYNI KALIYOR) ---
    uint16_t rx_buf[2];
    HAL_SPI_Receive(enc->hspi, (uint8_t*)rx_buf, 2, 100);
    enc->raw_value = (((uint32_t)rx_buf[0] << 16) | rx_buf[1]) & 0x3FFFF;
    Delay_us(SSI_TIMEOUT_US);
#endif

    // Açı Hesaplama (Ortak)
    // 18-bit için katsayı: 360 / 262144 = 0.00137329
    enc->angle_degrees = (float)enc->raw_value * 0.00137329f;
}
