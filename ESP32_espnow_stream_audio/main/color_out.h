#define COLOR_PRINT_BLACK "30"
#define COLOR_PRINT_RED "31"
#define COLOR_PRINT_GREEN "32"
#define COLOR_PRINT_BROWN "33"
#define COLOR_PRINT_BLUE "34"
#define COLOR_PRINT_PURPLE "35"
#define COLOR_PRINT_CYAN "36"
#define color_printf(COLOR, format, ...)                               \
    {                                                                  \
        printf("\033[0;" COLOR "m" format "\033[0m\n", ##__VA_ARGS__); \
    }