#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT "/dev/ttyACM0"
#define DEFAULT_BAUD B1152000

// global variables
volatile int running = 1;
int serial_port;

// function prototypes
void handle_signal(int sig);
void configure_terminal(void);
void restore_terminal(void);

// function to configure terminal for immediate input
void configure_terminal() {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and echo
    tcsetattr(0, TCSANOW, &term);
}

// function to restore terminal to normal mode
void restore_terminal() {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag |= (ICANON | ECHO); // enable canonical mode and echo
    tcsetattr(0, TCSANOW, &term);
}

// signal handler function
void handle_signal(int sig) {
    if (sig == SIGINT) {
        const char *exit_message = "-1;0;;;;;;0;\n";
        write(serial_port, exit_message, strlen(exit_message));
        running = 0;
        restore_terminal();
        exit(0);
    }
}

// convert "mm:ss" format to seconds
int time_to_seconds(const char* time_str) {
    if (!time_str) return 0;

    int minutes, seconds;
    if (sscanf(time_str, "%d:%d", &minutes, &seconds) == 2) {
        return minutes * 60 + seconds;
    }
    return 0;
}

// execute playerctl command and get output
char* get_playerctl_data(const char* command) {
    char* buffer = malloc(BUFFER_SIZE);
    FILE* pipe = popen(command, "r");

    if (!pipe) {
        printf("Error executing playerctl command\n");
        free(buffer);
        return NULL;
    }

    if (fgets(buffer, BUFFER_SIZE, pipe) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; // remove newline
    } else {
        free(buffer);
        buffer = NULL;
    }

    pclose(pipe);
    return buffer;
}

// convert status string to integer code
int get_status_code(const char* status) {
    if (status == NULL) return -1;
    if (strcmp(status, "Playing") == 0) return 1;
    if (strcmp(status, "Paused") == 0) return 0;
    return -1;
}

// function to handle keyboard input
void *keyboard_thread(void *arg) {
    configure_terminal();
    char input;

    while (running) {
        if (read(STDIN_FILENO, &input, 1) == 1) {
            if (input == 'q' || input == 'Q') {
                const char *exit_message = "-1;0;;;;;;0;\n";
                write(serial_port, exit_message, strlen(exit_message));
                printf("\nExiting...\n");
                running = 0;
                break;
            }
        }
    }

    restore_terminal();
    return NULL;
}

// initialize serial port
int init_serial_port(const char *port, speed_t baud) {
    int serial_port = open(port, O_RDWR);

    if (serial_port < 0) {
        printf("Error opening serial port: %s\n", strerror(errno));
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error getting serial port attributes: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error setting serial port attributes: %s\n", strerror(errno));
        return -1;
    }

    return serial_port;
}

int main(int argc, char *argv[]) {
    char *port_path = DEFAULT_PORT;
    speed_t baud_rate = DEFAULT_BAUD;

    // set up signal handler
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // parse command line arguments
    int opt;
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"baud", required_argument, 0, 'b'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "p:b:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                port_path = optarg;
                break;
            case 'b':
                baud_rate = atoi(optarg);
                switch (baud_rate) {
                    case 9600: baud_rate = B9600; break;
                    case 115200: baud_rate = B115200; break;
                    case 1152000: baud_rate = B1152000; break;
                    default:
                        printf("Unsupported baud rate. Using default.\n");
                        baud_rate = DEFAULT_BAUD;
                }
                break;
        }
    }

    // initialize serial port
    serial_port = init_serial_port(port_path, baud_rate);
    if (serial_port < 0)
        return 1;

    // create thread for keyboard input
    pthread_t kbd_thread;
    if (pthread_create(&kbd_thread, NULL, keyboard_thread, NULL) != 0) {
        fprintf(stderr, "Error creating keyboard thread\n");
        return 1;
    }

    while (running) {
        // get all required data
        char* status_str = get_playerctl_data("playerctl --player=spotify status");
        char* volume_str = get_playerctl_data("playerctl --player=spotify volume");
        char* title = get_playerctl_data("playerctl --player=spotify metadata title");
        char* artist = get_playerctl_data("playerctl --player=spotify metadata artist");
        char* position_str = get_playerctl_data("playerctl --player=spotify metadata --format '{{duration(position)}}'");
        char* time_remaining = get_playerctl_data("playerctl --player=spotify metadata --format '{{duration(mpris:length - position)}}'");
        char* full_lenght = get_playerctl_data("playerctl --player=spotify metadata --format '{{duration(mpris:length)}}'");

        if (running) {  // check if we should continue processing
            // process status
            int status_code = get_status_code(status_str);

            // process volume (convert to percentage)
            float volume = volume_str ? atof(volume_str) * 100 : 0;

            // calculate song progress percentage
            int current_seconds = time_to_seconds(position_str);
            int total_seconds = time_to_seconds(full_lenght);
            int percentage = 0;

            if (total_seconds > 0) {
                percentage = (current_seconds * 100) / total_seconds;
                if (percentage > 100) percentage = 100;
                if (percentage < 0) percentage = 0;
            }

            // prepare safe strings for title and artist
            char safe_title[BUFFER_SIZE] = "Unknown";
            char safe_artist[BUFFER_SIZE] = "Unknown";

            if (title && strlen(title) > 0) {
                strncpy(safe_title, title, BUFFER_SIZE - 1);
                safe_title[BUFFER_SIZE - 1] = '\0';
            }
            if (artist && strlen(artist) > 0) {
                strncpy(safe_artist, artist, BUFFER_SIZE - 1);
                safe_artist[BUFFER_SIZE - 1] = '\0';
            }

            // format message for Serial
            char message[BUFFER_SIZE];
            snprintf(message, BUFFER_SIZE, "%d;%.0f%%;%s;%s;%s;-%s;%s;%d;\n",
                    status_code, volume, safe_title, safe_artist, position_str,
                    time_remaining, full_lenght, percentage);

            // send to Serial
            write(serial_port, message, strlen(message));
            printf("media data: %s", message);
        }

        // clean up
        free(status_str);
        free(volume_str);
        free(title);
        free(artist);
        free(position_str);
        free(time_remaining);
        free(full_lenght);

        // wait before next update
        usleep(100000);  // update every 100ms
    }

    // cleanup
    pthread_join(kbd_thread, NULL);
    restore_terminal();
    close(serial_port);
    return 0;
}
