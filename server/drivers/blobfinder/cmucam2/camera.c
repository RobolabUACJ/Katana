#ifdef _cplusplus
extern "C"
{
#endif

#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**************************************************************************
			    *** CONSTANST ***
**************************************************************************/
#define IMAGE_WIDTH      83             // the width of the frame camera sends
#define IMAGE_HEIGHT     143            // the height of the frame camera sends
#define CONTRAST         5              // camera's contrast register #
#define BRIGHTNESS       6		          // camera's brightness register #
#define COLORMODE        18		          // camera's colormode register #
#define RGB_AWT_ON       44             // camera's RGB auto white balance on
#define RGB_AWT_OFF      40             // camera'sRGB auto white balance off
#define YCRCB_AWT_ON     36             // camera'sYCrCb auto white balance on
#define YCRCB_AWT_OFF    32             // camera'sYCrCb auto white balance off
#define AUTOGAIN         19		          // camera's autogain register #
#define AUTOGAIN_ON      33             // camera's autogain on
#define AUTOGAIN_OFF     32             // camera's autogain off                
#define ZERO_POSITION    128            // servos' middle position as defiend by camera
#define MIN_RGB          16		          // camera's min rgb value
#define MAX_RGB          240		        // camera's max rgb value
#define T_PACKET_LENGTH  33             // max length of T packet that camera returns

typedef unsigned int     uint32_t;
typedef unsigned short   uint16_t;
typedef unsigned char    uint8_t;

/**************************************************************************
			                      *** T PACKET ***
**************************************************************************/
typedef struct                          // camera's output packet for tracking blobs
{
   int middle_x, middle_y; 		          // the blob entroid (image coords)
   int left_x;				                  // the left most corner's x value
   int left_y;				                  // the left msot corner's y value
   int right_x;				                  // the right most corner's x vlaue
   int right_y;				                  // the right most corner's y value
   int blob_area;	        	            // number of pixles int he tracked regtion,
					                              // scaled and capped at 255:(pixles+4)/8
   int confidence;			                // the (# of pixles/area)*256 of the bounded
					                              // rectangle and capped at 255
}packet_t; 

/**************************************************************************
			                    *** IMAGER CONFIG ***
**************************************************************************/
typedef struct                           // camera's internal register controlling image quality
{ 
   uint8_t subtype;                      // must be PLAYER_BLOBFINDER_SET_IMAGER_PARAMS_REQ.
   int16_t brightness;                   // contrast:      -1 = no change.  (0-255)
   int16_t contrast;                     // brightness:    -1 = no change.  (0-255)
   int8_t  colormode;                    // color mode:    -1 = no change.
   	                                 //		             0  = RGB/auto white balance Off,
                                         //                1  = RGB/AutoWhiteBalance On,
                                         //                2  = YCrCB/AutoWhiteBalance Off, 
                                         //                3  = YCrCb/AWB On)  
  int8_t  autogain;                      // auto gain:     -1 = no change.
		                         //		   0  = off, 
                               		 //		   1  = on.         
} imager_config;

/**************************************************************************
			                    *** CONFIG CONFIG ***
**************************************************************************/
typedef struct
{ 
  uint8_t subtype;                      // must be PLAYER_BLOBFINDER_SET_COLOR_REQ.
  int16_t rmin, rmax;                   // RGB minimum and max values (0-255)
  int16_t gmin, gmax;
  int16_t bmin, bmax;
} color_config;

/**************************************************************************
			                        *** RGB ***
**************************************************************************/
typedef struct                          // RGB values
{
	int red;
	int green;
	int blue;
} rgb;

/**************************************************************************
			                    *** CONFIG CONFIG ***
**************************************************************************/
typedef struct                          // camera's image
{
  int width;
  int height;
  rgb **pixel;
} image;

color_config range;

/**************************************************************************
			                      *** FUNCTION PROTOTYPES ***
**************************************************************************/
void get_t_packet(int fd, packet_t *tpacket);
int  set_imager_config(int fd, imager_config ic);
int  get_bytes(int fd, char *buf, size_t len);
int  open_port(char *devicepath);
void close_port(int fd);
void read_t_packet(int fd, char *tpackChars);
void set_t_packet( packet_t *tpacket, char output[] );
int  set_servo_position(int fd, int servo_num, int angle);
int  get_servo_position(int fd, int servo_num);
void stop_tracking(int fd);
int  write_check(int fd, char *msg);
int  poll_mode(int fd, int on);
void make_command(char *cmd, int *n, size_t size, char *fullCommand);
void track_blob(int fd, color_config cc);

int power(int fd, int on)
{
  if(on)
    return write_check(fd, "CP 1\r");
  return write_check(fd, "CP 0\r");
}

/**************************************************************************
			                      *** SET IMAGER CONFIG ***
**************************************************************************/
/* Description: This function sets the camera's internal resiger values
                for controlling image qualities.
   Parameters:  fd: serial port file descriptor
                player_blobfinder_imager_config: a Player packet containing
                information for camera's internal register:
                contrast, brightness, color mode, Exposure
   Returns:     1: If the command was successfully sent to the camera
                0: Otherwise
*/

int set_imager_config(int fd, imager_config ic)
{
   int value[8], size = 0;                    // The numbers used in the command: 
   char command[26];                          // ex. CR 5 255 19 33
   if(ic.contrast != -1)                      // If ther is a change set the values
   {
      value[size++] = CONTRAST;
      value[size++] = ic.contrast;
   }
   if(ic.brightness != -1)
   {
      value[size++] = BRIGHTNESS;
      value[size++] = ic.brightness;
   }
   if(ic.colormode != -1)
   {
      value[size++] = COLORMODE;
      if(ic.colormode == 0)
      	value[size++] = RGB_AWT_OFF;
      if(ic.colormode == 1)
      	value[size++] = RGB_AWT_ON;
      if(ic.colormode == 2)
      	value[size++] = YCRCB_AWT_OFF;
      if(ic.colormode == 3)
      	value[size++] = YCRCB_AWT_ON;
   }
   if(ic.autogain != -1)
   {
      value[size++] = AUTOGAIN;
      if(ic.autogain == 0)
      	value[size++] = AUTOGAIN_OFF;
      if(ic.autogain == 1)
      	value[size++] = AUTOGAIN_ON;
   }
   make_command("CR ", value, size, command);  // Put the values into camera's command format:
                                               // ex. CR 6 105 18 44
   return write_check(fd, command);            // send the command to the camera
}

/**************************************************************************
			                      *** GET T PACKET ***
**************************************************************************/
/* Description: This function puts the camera's output during tracking into
                a T packet, which contrains information about the blob.
   Parameters:  fd: serial port file descriptor
                tpacket: the packet that will contain the blob info
   Returns:     void
*/
void get_t_packet(int fd, packet_t *tpacket)
{
  char tpack_chars[T_PACKET_LENGTH];
  read_t_packet(fd, tpack_chars);                   // read the output of the camera
  set_t_packet(tpacket, tpack_chars);               // convert it into T packet
}

/**************************************************************************
			                      *** POLL MODE ***
**************************************************************************/
/* Description: This functions determines whether the camera should send a
                continuous stream of packets or just one packet.
   Parameters:  fd: serial port file descriptor
                on: if on == 1, only one packet is send
                    if on == 0, a continuous stream of packets is send
   Returns:     1: If the command was successfully sent to the camera
                0: Otherwise
*/
int poll_mode(int fd, int on)
{
  if(on)
    return write_check(fd, "PM 1\r");
  else
    return write_check(fd, "PM 0\r");
}
   
/**************************************************************************
			                      *** SET SERVO POSITION ***
**************************************************************************/
/* Description: This functions sets the servo position given the servo number
                and the angle (note: angle = 0 denotes servo position = 128
                in terms of camera's values)
   Parameters:  fd: serial port file descriptor
                servo_num: the servo which we are setting the position
                I am using 0:pan  1:tilt
   Returns:     1: If the command was successfully sent to the camera
                0: Otherwise
*/
int set_servo_position(int fd, int servo_num, int angle)
{
   int position = ZERO_POSITION + angle;                      // change the angle into camera's format
   char comm[10];                                             // for servo position. I am using angle 0
   int value[] = {servo_num, position};                       // corresponding to the default servo pos. 128
   make_command("SV ", value, sizeof(value)/sizeof(int), comm);  // generate the command using the values
   printf("servo %d new position: %d\n", servo_num, angle);
   return write_check(fd, comm);                              // write the command to the camera
}

/**************************************************************************
			                      *** MAKE COMMAND ***
**************************************************************************/
/* Description: This function gets a sets of values and a camera command header
                to generate the command for the camera.
   Parameters:  cmd: the command header, for example SF or CR (see CMUcam 2 user guide)
                n: the set of values to be used in the command
                size: the number of values used
                full_command: the final command in characters to be send to the camera
   Returns:     void
*/
void make_command(char *cmd, int *n, size_t size, char *full_command)
{
  char value[3];                            // the values are all withing 3 digits
  int length, i;
  for(i = 0; i < 10; i++)                   // set all to null so that if there are
    full_command[i] = '\0';                 // unsed characters at the end, the camera
                                            // does not complain about the command.
                                            // there is probably a better way to do this!
  strcat(full_command, cmd);                // attach the command header, ex. SF
  for(i = 0; i < (int)size; i++)                 // for all the values, convert them into char
  {                                         // and attach them to the end of the command
    length = sprintf(value, "%d", n[i]);    // plus a space
    strcat(full_command, value);
    strcat(full_command, " ");
  }
  strcat(full_command, "\r");               // attach the return character to the end
}

/**************************************************************************
			                      *** OPEN PORT ***
**************************************************************************/
/* Description: This function opens the serial port for communication with the camera.
   Parameters:  NONE
   Returns:     the file descriptor
*/
int open_port(char *devicepath)
{
   int fd = open( devicepath, O_RDWR );             // open the serial port
   struct termios term;
 
   if( tcgetattr( fd, &term ) < 0 )                 // get device attributes
   {
        puts( "unable to get device attributes");
        return -1;
   }
  
#if HAVE_CFMAKERAW
   cfmakeraw( &term );
#endif
   
  cfsetispeed( &term, B115200 );                   // set baudrate to 115200
   cfsetospeed( &term, B115200 );                          
  
   if( tcsetattr( fd, TCSAFLUSH, &term ) < 0 )
   {
        puts( "unable to set device attributes");
        return -1;
   }

  // Make sure queue is empty
  tcflush(fd, TCIOFLUSH);

   return fd;
}

/**************************************************************************
			                      *** CLOSE PORT ***
**************************************************************************/
/* Description: This function closes the serial port.
   Parameters:  NONE
   Returns:     void
*/
void close_port(int fd)
{               
	close(fd);
}

/**************************************************************************
			                      *** WRITE CHECK ***
**************************************************************************/
/* Description: This function writes a command to the camera and checks if the
                write was done successfully by checking camera's response.
   Parameters:  fd: serial port file descriptor
                msg: the command to be send to the camera
   Returns:     1: if the write was successful
                0: otherwise
*/
int write_check(int fd, char *msg)
{
  char respond[5];
  
  if( write(fd, msg, strlen(msg)) != (int)strlen(msg) )   // write the command to the camera
  {
    printf( "ERROR: writing to serial device failed.\n" );
    return 0;
  }

  if( get_bytes(fd, respond, 5) < 1 )
  {
    printf( "ERROR: get bytes failed\n" );
    return 0;
  }

  if(respond[0] == 'N')                // If NCK is returned, there was an error in writi
  {
     printf("ERROR: received NCK!\n");
     return 0;
  }
  return 1;
}

int get_bytes(int fd, char *buf, size_t len)
{
  int bytes_read = 0, ret_val;
  while(bytes_read < (int)len)
  {
    ret_val = read(fd, buf+bytes_read, len-bytes_read);
    if(ret_val < 0)
    {
      perror("ERROR: getting bytes failed.\n");
      return 0;
    }
    else if(ret_val > 0)
      bytes_read += ret_val;
  }
  return bytes_read;
}
    
int get_servo_position(int fd, int servo_num)
{  
   int servo_position;
   int i;
   char number[3];
   char c = 0;
   
   if(servo_num)       // set position of servo 1
     write(fd, "GS 1\r", 5);
   else		             // set position of servo 0   
     write(fd, "GS 0\r", 5);

   for(i = 0; i < 4; i++)
     read(fd, &c, 1);

   for(i = 0; 1; i++)
   {
     read(fd, &c, 1);
     if(c == '\r')
       break;     
     number[i] = c;
   }
   servo_position = atoi(number);
   return (servo_position - ZERO_POSITION);
}

/* This functions starts to Track a Color. It takes in the minimum and 
   maximum RGB values and outputs a type T packet. This packet by defualt
   returns the middle mass x and y coordinates, the bounding box, the 
   number of pixles tracked, and a confidence values. 
*/
void track_blob( int fd, color_config cc)
{
   char cmd[28];
   int value[] = {cc.rmin, cc.rmax, cc.gmin, cc.gmax, cc.bmin, cc.bmax};
   range = cc;
   
   // Enabling Auto Servoing Mode for both servos             
   /*if(!write_check(fd, "SM 15\r"))
   {
     printf("Auto servoing failed.\n");
     return;
     }*/
   make_command("TC ", value, sizeof(value)/sizeof(int), cmd); 
   if(!write_check(fd, cmd))
   {
     printf("ERROR; track color failed.\n");
     return;
   }
}

void stop_tracking(int fd)
{
  char c = 0;
  write(fd, "\r", 1);
  while(c != ':')
     read(fd, &c, 1);
}

void read_t_packet(int fd, char *tpack_chars)
{
   char c = 0;
   int k = 0;
   while(1)
   {
     read(fd, &c, 1);
     tpack_chars[k++] = c;
     if(c == '\r')
	break;
   }
   if(tpack_chars[k-1] != '\r')
     printf("ERROR: reading T packet failed.\n");   
}

// Extracts the data for type T packet from camera output.
void set_t_packet( packet_t *tpacket, char output[] )
{
   sscanf(output, "%d %d %d %d %d %d %d %d", &tpacket->middle_x, &tpacket->middle_y,
	  &tpacket->left_x, &tpacket->left_y, &tpacket->right_x, &tpacket->right_y,
	  &tpacket->blob_area, &tpacket->confidence);	
}