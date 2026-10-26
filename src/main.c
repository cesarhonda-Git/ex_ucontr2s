#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(race_condition_demo, LOG_LEVEL_INF);

typedef struct {
    uint32_t pulse_count;
    uint32_t calculated_flow;
} sensor_telemetry_t;

static volatile sensor_telemetry_t g_sensor_data = {0, 0};

/* ISR do Timer executando a cada 100 microsegundos */
static void sensor_isr_handler(struct k_timer *timer_id)
{
    static uint32_t count = 0;
    count++;

    g_sensor_data.pulse_count = count;
    g_sensor_data.calculated_flow = count * 10;
}

K_TIMER_DEFINE(pulse_timer, sensor_isr_handler, NULL);

int main(void)
{
    LOG_INF("=== Demonstracao de Race Condition (Zephyr 4.2 - FRDM-KL25Z) ===");
    LOG_INF("Iniciando timer de alta frequencia em contexto ISR...");

    /* Dispara a ISR a cada 100 microsegundos */
    k_timer_start(&pulse_timer, K_USEC(100), K_USEC(100));

    uint32_t race_detection_counter = 0;
    uint32_t loop_counter = 0;

    while (1) {
        /* 1. Le o primeiro valor da estrutura */
        uint32_t local_pulses = g_sensor_data.pulse_count;

        /* 2. Mantem a CPU ocupada por 50us para forcar a interrupcao no meio da amostragem */
        k_busy_wait(50);

        /* 3. Le o segundo valor da estrutura (pode ter sido alterado pela ISR) */
        uint32_t local_flow = g_sensor_data.calculated_flow;

        /* Validação de consistência */
        uint32_t expected_flow = local_pulses * 10;

        if (local_flow != expected_flow) {
            race_detection_counter++;
            LOG_ERR("[RACE CONDITION #%u DETECTADA] Pulso: %u | Vazao Lida: %u (Esperado: %u)",
                    race_detection_counter, local_pulses, local_flow, expected_flow);
        }

        loop_counter++;

        /* Libera a CPU brevemente a cada 5.000 amostragens para nao travar o Zephyr e flush do Log */
        if (loop_counter % 5000 == 0) {
            k_msleep(1);
        }
    }

    return 0;
}