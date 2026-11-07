/* Bare-metal RISC-V demo for QEMU's "virt" machine. */

#define UART0        ((volatile unsigned char *)0x10000000)  /* NS16550 UART */
#define TEST_FINISH  ((volatile unsigned int  *)0x00100000)  /* QEMU exit device */

int initialised = 42;   /* lands in .data */
int zeroed;             /* lands in .bss, cleared by start.S */

static void uart_putc(char c)
{
    *UART0 = (unsigned char)c;
}

static void uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

static void uart_hex(unsigned int v)
{
    const char digits[] = "0123456789abcdef";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4)
        uart_putc(digits[(v >> i) & 0xf]);
}

int main(void)
{
    uart_puts("Hello from bare-metal RISC-V!\n");

    uart_puts("initialised (.data) = ");
    uart_hex((unsigned int)initialised);
    uart_puts("\nzeroed      (.bss)  = ");
    uart_hex((unsigned int)zeroed);
    uart_puts("\n");

    /* A tiny loop that is nice to single-step in GDB */
    unsigned int sum = 0;
    for (unsigned int i = 1; i <= 10; i++)
        sum += i;

    uart_puts("sum 1..10 = ");
    uart_hex(sum);
    uart_puts("\n");

    /* Tell QEMU to exit cleanly (0x5555 = success) */
    *TEST_FINISH = 0x5555;

    for (;;)
        ;
}
