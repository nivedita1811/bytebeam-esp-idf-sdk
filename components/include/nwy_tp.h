#ifndef __NWY_TP_H__
#define __NWY_TP_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef uint8_t
typedef unsigned char uint8_t;
#endif

#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

#define I2C_1  "I2C1"
#define I2C_2  "I2C2"
#define I2C_3  "I2C3"

typedef enum
{
    TP_I2C_BPS_100K, ///< normal 100Kbps
    TP_I2C_BPS_400K, ///< fast 400Kbps
    TP_I2C_BPS_3P5M, ///< high speed 3.5Mbps
} tp_i2c_bps_t;

typedef enum {
    tp_irq_rising = 0,
    tp_irq_falling,
    tp_irq_rising_falling,
    tp_irq_high,
    tp_irq_low
} tp_irq_mode_t;

typedef void(*nwy_irq_handle)(int param);

void nwy_tp_power(unsigned int power_id, unsigned int mv, bool enable);

int nwy_tp_cst816_init(unsigned int rst_gpio, char *bus_name, tp_i2c_bps_t busBps, 
	unsigned int int_gpio, tp_irq_mode_t irq_mode, nwy_irq_handle cb);

int nwy_tp_read_byte(uint8_t slave_addr, uint16_t reg_addr, uint8_t *buf, int length);

int nwy_tp_write_byte(uint8_t slave_addr, uint16_t reg_addr, uint8_t *buf, int length);

#ifdef __cplusplus
   }
#endif

#endif 

