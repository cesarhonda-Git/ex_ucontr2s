#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h> 
#include <zephyr/drivers/i2c.h>
#include <stdlib.h>


// === Endereço e registradores do MMA8451Q ===
#define MMA8451Q_I2C_ADDR    0x1D
#define MMA8451Q_CTRL_REG1   0x2A

// === Bits de configuração ===
#define MMA8451Q_ACTIVE_BIT  0x01
#define MMA8451Q_ODR   (0x0 << 3)  // 100 Hz conforme datasheet (DR=100b)

// === Obter dispositivos ===
static const struct device *const accel = DEVICE_DT_GET(DT_ALIAS(accel0));
static const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

void mma8451q_configurar_odr(void)
{
    uint8_t buf[2];
    int ret;

    // 1️⃣ Colocar o sensor em standby (necessário antes de mudar ODR)
    buf[0] = MMA8451Q_CTRL_REG1;
    buf[1] = 0x00;
    ret = i2c_write(i2c_dev, buf, 2, MMA8451Q_I2C_ADDR);
    if (ret) {
        printk("ERRO ao colocar MMA8451Q em standby (%d)\n", ret);
        return;
    }

    // 2️⃣ Configurar ODR = 100 Hz (bits DR[5:3] = 100)
    buf[0] = MMA8451Q_CTRL_REG1;
    buf[1] = MMA8451Q_ODR;
    ret = i2c_write(i2c_dev, buf, 2, MMA8451Q_I2C_ADDR);
    if (ret) {
        printk("ERRO ao configurar ODR (%d)\n", ret);
        return;
    }

    // 3️⃣ Ativar o sensor novamente
    buf[0] = MMA8451Q_CTRL_REG1;
    buf[1] = MMA8451Q_ODR | MMA8451Q_ACTIVE_BIT;
    ret = i2c_write(i2c_dev, buf, 2, MMA8451Q_I2C_ADDR);
    if (ret) {
        printk("ERRO ao ativar MMA8451Q (%d)\n", ret);
        return;
    }

    printk("MMA8451Q configurado para 100 Hz via I2C.\n");
}

#define PRIO_THREAD_A 5   // Maior prioridade (número menor)
#define PRIO_THREAD_B 7   // Menor prioridade
static struct sensor_value ax, ay, az; // uso de variaves globais pois mais de uma thread usa essas variaveis
static int64_t delta_t;


void thread_A(void *p1, void *p2, void *p3)
{
    int ret;
    int64_t t_ms, t_ant = 0;
    while (1) {
        ret = sensor_sample_fetch(accel);
        if (ret) {
            printk("Erro sensor_sample_fetch (%d)\n", ret);
            k_msleep(20);
            continue;
        }

        t_ms = k_uptime_get();
        delta_t = t_ms - t_ant;
        t_ant = t_ms;

        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &ax);
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &ay);
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &az);
        k_msleep(5);
    }
}

void thread_B(void *p1, void *p2, void *p3)
{
    while (1) {
        printk("ΔT=%3lld ms | X=%d.%06d | Y=%d.%06d | Z=%d.%06d\n",
               delta_t,
               ax.val1, abs(ax.val2),
               ay.val1, abs(ay.val2),
               az.val1, abs(az.val2));

        // Intervalo aproximado para 100 Hz (10 ms por amostra)
        k_msleep(5);
    }
}

K_THREAD_DEFINE(a_tid, 512, thread_A, NULL, NULL, NULL, PRIO_THREAD_A, 0, 0);
K_THREAD_DEFINE(b_tid, 512, thread_B, NULL, NULL, NULL, PRIO_THREAD_B, 0, 0);



void main(void)
{
    //struct sensor_value ax, ay, az;
    //int ret;
    //int64_t t_ms, t_ant = 0;

    if (!device_is_ready(accel)) {
        printk("Sensor MMA8451Q não está pronto!\n");
        return;
    }

    if (!device_is_ready(i2c_dev)) {
        printk("I2C não está pronto!\n");
        return;
    }

    printk("Configurando ODR via I2C...\n");
    mma8451q_configurar_odr();

    // Pequeno atraso para estabilizar após reativar
    k_msleep(200);

    printk("Iniciando leituras contínuas...\n");
    while (1) {
    k_sleep(K_FOREVER); // Main dorme para liberar CPU.
    }
/* 
    while (1) {
        ret = sensor_sample_fetch(accel);
        if (ret) {
            printk("Erro sensor_sample_fetch (%d)\n", ret);
            k_msleep(20);
            continue;
        }

        t_ms = k_uptime_get();
        int64_t delta_t = t_ms - t_ant;
        t_ant = t_ms;

        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &ax);
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &ay);
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &az);

        printk("ΔT=%3lld ms | X=%d.%06d | Y=%d.%06d | Z=%d.%06d\n",
               delta_t,
               ax.val1, abs(ax.val2),
               ay.val1, abs(ay.val2),
               az.val1, abs(az.val2));

        // Intervalo aproximado para 100 Hz (10 ms por amostra)
        k_msleep(5);
    } */
}
