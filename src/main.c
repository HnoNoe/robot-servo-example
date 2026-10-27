/*
 * STM32F103ZET6 dual SG90 + USART1 (USB_232)
 * Commands (end with Enter):
 *   S1 / S2     toggle INIT <-> ACTIVE(inward 60deg)
 *   S1:0 S1:1   S2:0 S2:1
 *   HOME
 * PA6=left TIM3_CH1, PA7=right TIM3_CH2
 */
#include <stdint.h>

#define RCC_BASE          0x40021000UL
#define FLASH_R_BASE      0x40022000UL
#define GPIOA_BASE        0x40010800UL
#define GPIOB_BASE        0x40010C00UL
#define USART1_BASE       0x40013800UL
#define TIM3_BASE         0x40000400UL

#define RCC_CR            (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define FLASH_ACR         (*(volatile uint32_t *)(FLASH_R_BASE + 0x00))

#define GPIOA_CRL         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_CRH         (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOB_CRL         (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_ODR         (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

#define USART1_SR         (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR         (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR        (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1        (*(volatile uint32_t *)(USART1_BASE + 0x0C))

#define TIM3_CR1          (*(volatile uint32_t *)(TIM3_BASE + 0x00))
#define TIM3_CCMR1        (*(volatile uint32_t *)(TIM3_BASE + 0x18))
#define TIM3_CCER         (*(volatile uint32_t *)(TIM3_BASE + 0x20))
#define TIM3_PSC          (*(volatile uint32_t *)(TIM3_BASE + 0x28))
#define TIM3_ARR          (*(volatile uint32_t *)(TIM3_BASE + 0x2C))
#define TIM3_CCR1         (*(volatile uint32_t *)(TIM3_BASE + 0x34))
#define TIM3_CCR2         (*(volatile uint32_t *)(TIM3_BASE + 0x38))
#define TIM3_EGR          (*(volatile uint32_t *)(TIM3_BASE + 0x14))

#define SERVO1_INIT_DEG   180
#define SERVO1_ACTIVE_DEG 120
#define SERVO2_INIT_DEG   0
#define SERVO2_ACTIVE_DEG 60

static uint8_t s1_active;
static uint8_t s2_active;
static char rx_line[64];
static uint8_t rx_len;

static int streq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a++ != *b++) return 0;
    }
    return *a == *b;
}

static void SystemInit_Clock(void)
{
    RCC_CR |= (1UL << 16);
    while (!(RCC_CR & (1UL << 17))) {}

    FLASH_ACR = (1UL << 4) | 2UL;

    RCC_CFGR &= ~((0xFUL << 18) | (1UL << 16) | (0x7UL << 8) | (0x7UL << 11) | (0x3UL << 0));
    RCC_CFGR |= (7UL << 18) | (1UL << 16) | (4UL << 8);

    RCC_CR |= (1UL << 24);
    while (!(RCC_CR & (1UL << 25))) {}

    RCC_CFGR |= (2UL << 0);
    while (((RCC_CFGR >> 2) & 0x3UL) != 0x2UL) {}
}

static void Led_Init(void)
{
    RCC_APB2ENR |= (1UL << 3);
    GPIOB_CRL &= ~(0xFUL << 20);
    GPIOB_CRL |= (0x3UL << 20);
    GPIOB_ODR |= (1UL << 5);
}

static void Led_Toggle(void)
{
    GPIOB_ODR ^= (1UL << 5);
}

static void Usart1_Init(void)
{
    RCC_APB2ENR |= (1UL << 2) | (1UL << 14);
    GPIOA_CRH &= ~((0xFUL << 4) | (0xFUL << 8));
    GPIOA_CRH |= (0xBUL << 4) | (0x4UL << 8);
    USART1_BRR = 0x271; /* 115200 @72MHz */
    USART1_CR1 = (1UL << 13) | (1UL << 3) | (1UL << 2);
}

static void Usart1_WriteChar(char c)
{
    while (!(USART1_SR & (1UL << 7))) {}
    USART1_DR = (uint32_t)c;
}

static void Usart1_Write(const char *s)
{
    while (*s) Usart1_WriteChar(*s++);
}

static int Usart1_ReadChar(char *out)
{
    if (USART1_SR & (1UL << 5)) {
        *out = (char)(USART1_DR & 0xFF);
        return 1;
    }
    return 0;
}

static uint16_t AngleToPulse(uint16_t deg)
{
    if (deg > 180) deg = 180;
    return (uint16_t)(500U + ((uint32_t)deg * 2000U) / 180U);
}

static void Servo_Apply(void)
{
    uint16_t a1 = s1_active ? SERVO1_ACTIVE_DEG : SERVO1_INIT_DEG;
    uint16_t a2 = s2_active ? SERVO2_ACTIVE_DEG : SERVO2_INIT_DEG;
    TIM3_CCR1 = AngleToPulse(a1);
    TIM3_CCR2 = AngleToPulse(a2);
}

static void Servo_PwmInit(void)
{
    RCC_APB2ENR |= (1UL << 2);
    RCC_APB1ENR |= (1UL << 1);

    GPIOA_CRL &= ~((0xFUL << 24) | (0xFUL << 28));
    GPIOA_CRL |= (0xBUL << 24) | (0xBUL << 28);

    TIM3_PSC = 72 - 1;
    TIM3_ARR = 20000 - 1;
    TIM3_CCR1 = AngleToPulse(SERVO1_INIT_DEG);
    TIM3_CCR2 = AngleToPulse(SERVO2_INIT_DEG);
    TIM3_CCMR1 = (6UL << 4) | (1UL << 3) | (6UL << 12) | (1UL << 11);
    TIM3_CCER = (1UL << 0) | (1UL << 4);
    TIM3_EGR = 1UL;
    TIM3_CR1 = (1UL << 7) | (1UL << 0);
}

static void trim_line(char *s)
{
    uint8_t n = 0;
    while (s[n]) n++;
    while (n && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ')) {
        s[--n] = 0;
    }
}

static void HandleLine(char *line)
{
    trim_line(line);
    if (line[0] == 0) return;

    if (streq(line, "S1")) s1_active ^= 1U;
    else if (streq(line, "S2")) s2_active ^= 1U;
    else if (streq(line, "S1:0")) s1_active = 0;
    else if (streq(line, "S1:1")) s1_active = 1;
    else if (streq(line, "S2:0")) s2_active = 0;
    else if (streq(line, "S2:1")) s2_active = 1;
    else if (streq(line, "HOME")) { s1_active = 0; s2_active = 0; }
    else { Usart1_Write("ERR\r\n"); return; }

    Servo_Apply();
    Usart1_Write("OK S1=");
    Usart1_WriteChar(s1_active ? '1' : '0');
    Usart1_Write(" S2=");
    Usart1_WriteChar(s2_active ? '1' : '0');
    Usart1_Write("\r\n");
    Led_Toggle();
}

int main(void)
{
    char c;
    SystemInit_Clock();
    Led_Init();
    Usart1_Init();
    Servo_PwmInit();
    s1_active = 0;
    s2_active = 0;
    Servo_Apply();
    Usart1_Write("SERVO READY\r\n");

    for (;;) {
        if (Usart1_ReadChar(&c)) {
            if (c == '\n' || c == '\r') {
                if (rx_len > 0) {
                    rx_line[rx_len] = 0;
                    HandleLine(rx_line);
                    rx_len = 0;
                }
            } else if (rx_len < sizeof(rx_line) - 1) {
                rx_line[rx_len++] = c;
            } else {
                rx_len = 0;
            }
        }
    }
}

void SystemInit(void) {}
