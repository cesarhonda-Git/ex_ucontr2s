#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/sys/printk.h>

#include <zephyr/device.h>

/* #include <zephyr/drivers/sensor.h>

#include <zephyr/drivers/i2c.h>

#include <stdlib.h> */

#define PRIO_THREAD_A 5   // Maior prioridade (número menor) padeiro

#define PRIO_THREAD_B 5   // Menor prioridade (numero maior) clientes


volatile int saldo_vitrine = 0; // quantidadea de paos na vitrine

void thread_A(void *p1, void *p2, void *p3) //padeiro
{
    int pao = 0;
    while (1) {
        if (saldo_vitrine < 10)
        {
            pao++;
            saldo_vitrine++;
            printk("PADEIRO: Pão #%d pronto (Saldo vitrine = %d)\n", pao, saldo_vitrine);
        }
        else
        {
            printk("PADEIRO: Vitrine cheia ================================\n");
        }
        k_msleep(1000);
        }

}



void thread_B(void *p1, void *p2, void *p3) //clientes
{
    int pao = 0;
    while (1) {
        if (saldo_vitrine > 0)
        {
            pao++;
            saldo_vitrine--;
            printk("CLIENTE: Peguei pão #%d (Saldo vitrine = %d)\n", pao, saldo_vitrine);
            k_msleep(1500);    
        }
        else
        {
            printk("CLIENTE: Vitrine vazia ================================\n");
        }
    }
}

K_THREAD_DEFINE(a_tid, 512, thread_A, NULL, NULL, NULL, PRIO_THREAD_A, 0, 0);

K_THREAD_DEFINE(b_tid, 512, thread_B, NULL, NULL, NULL, PRIO_THREAD_B, 0, 0);

void main(void)

{
    printk("Padaria aberta...\n");

    while (1) {

    k_sleep(K_FOREVER); // Main dorme para liberar CPU.

    }
}