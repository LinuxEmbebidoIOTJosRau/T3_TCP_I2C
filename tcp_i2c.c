#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <string.h>

#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define SENSOR_ADDR     0x18

#define PORT 8080

#define MSG_SIZE 28
#define REPETITIONS 3

static int read_registers(int fd, uint8_t addr, uint8_t reg, uint8_t * buff, uint8_t size);

static int write_registers(int fd, uint8_t addr, uint8_t reg, uint8_t * buff, uint8_t size);


int main(int argc, char * argv[])
{
    uint8_t enable = 0x57;
    uint8_t acc[6] = {0};
    int8_t x_raw, y_raw, z_raw;
    float x_acc, y_acc, z_acc;
    char msg[MSG_SIZE];

    int fd = open("/dev/i2c-1", O_RDWR);

    int socket_connection = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    memset(&server, 0 , sizeof(server));
    
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int ret = bind(socket_connection, (struct  sockaddr *)&server, sizeof(server) );
    ret = listen(socket_connection, 100);

    // Se habilita el funcionamiento de los 3 ejes:
    ret = write_registers(fd, SENSOR_ADDR, 0x20, &enable, sizeof(enable));
    if (ret < 0)
    {
        return -1;
    }

    for (int i=0; i<REPETITIONS; i++)
    {
        int sock = accept(socket_connection, (struct sockaddr *)NULL, NULL);
        // Se lee el valor de aceleración de cada eje:
        ret = read_registers(fd, SENSOR_ADDR, 0x28, acc, sizeof(acc));
        // Reduciendo un poco la precisión, se puede tomar solo la parte alta de la aceleración:
        x_raw = acc[1];
        y_raw = acc[3];
        z_raw = acc[5];
        // Se convierten los datos fijos a aceleración gravitatoria:
        x_acc = ((float)x_raw)/62.0;
        y_acc = ((float)y_raw)/63.0;
        z_acc = ((float)z_raw)/65.0;
        sprintf(msg, "x: %.2f\ny: %.2f\nz: %.2f\n", x_acc, y_acc, z_acc);
        ret = send(sock, (void *)msg, strlen(msg), 0);
        ret = close(sock);
    }

    close(socket_connection);
    close(fd);

    return 0;
}

static int read_registers(int fd, uint8_t addr, uint8_t reg, uint8_t * buff, uint8_t size)
{
    
    struct i2c_rdwr_ioctl_data msgs[1];
    struct i2c_msg i2cmsg[2];
    int ret;
    uint8_t register_address = reg | (1 << 7);

    /*Send WR Register Address*/
    i2cmsg[0].addr = addr;
    i2cmsg[0].flags = 0;
    i2cmsg[0].len = 1;
    i2cmsg[0].buf = &register_address;

    i2cmsg[1].addr = addr;
    i2cmsg[1].flags = I2C_M_RD; 
    i2cmsg[1].len = size;
    i2cmsg[1].buf = buff;

    msgs[0].msgs = i2cmsg;
    msgs[0].nmsgs = 2;

    if ( (ret = ioctl(fd, I2C_RDWR, msgs)) < 0) {
        perror("ERROR in I2C_RDWR");
        close(fd);
        return -1;
    }

    return 0;
}

static int write_registers(int fd, uint8_t addr, uint8_t reg, uint8_t * buff, uint8_t size)
{

    char buffer[size + 1];

    if ( ioctl(fd, I2C_SLAVE, addr) < 0) {
        printf("ERROR in I2C_SLAVE\n");
        close(fd);
        return -1;
    }

    buffer[0] = reg;
    memcpy(&buffer[1]   , buff, size);

    /*Set Registers*/
    if (write(fd, &buffer, size + 1) < 0 ){
        perror("ERROR in WRITE\n");
        return -1;
    }
        
    return 0;
}