/*
 * fenac_encoder.h
 *
 * Created on: Jan 29, 2026
 * Author: User
 */

#ifndef INC_FENAC_ENCODER_H_
#define INC_FENAC_ENCODER_H_

#include "main.h" // HAL kütüphaneleri için gerekli

// --- KULLANICI AYARLARI ---
// Eğer donanım yoksa ve test yapılacaksa bu satırı yorumdan çıkarın:
//#define SPI_LOOPBACK_TEST_MODE

// Enkoder Çözünürlüğü (Datasheet'e göre 21 bit'e kadar çıkabilir )
#define ENCODER_RESOLUTION_BITS  18
#define ENCODER_MASK             0x3FFFF // 21 bit maskesi (Bütün bitler 1)

// SSI Monoflop Süresi (Datasheet: 20 us )
// Okumalar arası bekleme süresi (mikrosaniye)
#define SSI_TIMEOUT_US           25

// --- ENKODER YAPISI ---
typedef struct {
	SPI_HandleTypeDef *hspi;   // Kullanılan SPI Birimi (hspi1, hspi2 vb.)
	uint32_t raw_value;        // Ham veri (Binary Code)
	float angle_degrees;       // Dereceye çevrilmiş açı (0-360)
	uint32_t zero_offset;      // Sıfırlama noktası ofseti

	// Test modu değişkenleri
	uint32_t test_counter;
} FenacEncoder_t;

// --- FONKSİYON PROTOTİPLERİ ---
void Fenac_Init(FenacEncoder_t *enc, SPI_HandleTypeDef *hspi_handle);
void Fenac_Read_SSI(FenacEncoder_t *enc);
void Fenac_Set_Zero(FenacEncoder_t *enc);

#endif /* INC_FENAC_ENCODER_H_ */
