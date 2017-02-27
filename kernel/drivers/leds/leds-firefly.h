#define GPIO_0  0
#define GPIO_1  1
#define GPIO_2  2
#define GPIO_3  3
#define GPIO_4  4
#define GPIO_5  5
#define GPIO_6  6
#define GPIO_7  7

#define CMD_ON  0
#define CMD_OFF 1
#define CMD_KEY 2

#define KEY_DOWN 0
#define KEY_UP   1


static int g_gpio_0;
static int g_gpio_1;
static int g_gpio_2;
static int g_gpio_3;
static int g_gpio_4;
static int g_gpio_5;
static int g_gpio_6;
static int g_gpio_7;

static int g_board_key;



struct firefly_led_info {
	struct platform_device	*pdev;
	int     power_gpio;
	int     power_enable_value;
	int     work_gpio;
	int     work_enable_value;

	int	led_WifiHostWake;
	int	led_Gpio7A5D;
	int	led_CompInt;
	int	led_GryInt;
	int	led_LightInt;
	int	led_key;
	int	led_power;
	int	gpio_485;

	int	board_key;

	int	led_dm9620;
	int     dm9620_enable_value;
};

