#ifndef __MQTT_PACKET_WRITER_H__
#define __MQTT_PACKET_WRITER_H__

#include <stdbool.h>

#define MQTT_SERVER_CONNECTED 		true
#define MQTT_SERVER_DISCONNETED		false

#define MQTT_BUFF_SIZE	400
//****************************Connection Packet Variables***********************************
#define USERNAME_FLAG  0x01<<7
#define PASSWORD_FLAG  0x01<<6
#define WILL_RETAIN    0x01<<5
#define WILL_QOS1      0x01<<4
#define WILL_QOS0      0x01<<3
#define WILL_FLAG      0x01<<2
#define CLEAN_SESSION  0x01<<1

//***************************Publish Packet Variables*************************************/
#define RETAIN    0x01<<0
#define QosLEVEL0 0x01<<1
#define QosLEVEL1 0x01<<2
#define DUP       0x01<<3

//*************************Subscribe Packet Variables***********************************/
#define SUBQoSLEVEL0 0x01<<0
#define SUBQoSLEVEL1 0x01<<1

#define SERVER_CONN_PACKET_SIZE 100
#define SERVER_PUB_PACKET_SIZE	512
#define SERVER_SUB_PACKET_SIZE	100
#define SERVER_PING_PACKET_SIZE	50
#define SERVER_UNSUB_PACKET_SIZE 100
#define SERVER_DISCONN_PACKET_SIZE 100

#define CONN_ACK_FRAME_SIZE		100
#define PING_ACK_FRAME_SIZE		50
#define PUB_ACK_FRAME_SIZE		100
#define SUB_ACK_FRAME_SIZE		256
#define UNSUB_ACK_FRAME_SIZE	100
#define DISCONN_ACK_FRAME_SIZE	100

typedef enum{
 QosLevel0,
 QosLevel1,
 QosLevel2
}QosLevel;

typedef enum{
	NORMAL_PACKET=0,
	MQTT_CONN_PACKET=1,
	MQTT_PUBLISH_PACKET=2,
	MQTT_SUBSCRIBE_PACKET =3,
	MQTT_PING_PACKET=4,
	MQTT_UNSUBSCRIBE_PACKET=5,
	MQTT_DISCONN_PACKET=6
}packet_type_t;



typedef struct data_packet_{
	char data[MQTT_BUFF_SIZE];				//Pointer to data Buffer where the data is to be stored
	int totalSize;			//total size of the data stored in the buffer
	int nextPos;				//next position of the buffer to be written
	packet_type_t packetType; // packet type to be send
}data_packet_t;



typedef struct app_config_struct_{
	char MqttUsername[20];
	char MqttPassword[20];
	char clientID[15];
	char willTopic[20];
	char willMessage[20];
	char protocolName[10];
	unsigned char protocolType;
	QosLevel qosLevel;
	int retain;
	int dup;
	unsigned short keepAliveDurationSec;
}app_config_struct_t;

typedef bool mqtt_server_status_t;

/*==============================FUNCTION DECLARATIONS===================================================*/
/********************************************************************************************************
@function : void set_keepalive_duration(app_config_struct_t*, unsigned short )

@Description : This function is used to set the keep alive duration in seconds for mqtt connection.
@parameter	 :	app_config_struct_t* -> [in] -> This is address of the configuration structure used for storing 
												various parameters of the server
				unsigned short		      	 -> [in] -> this is the keepalive duration value to be set on mqtt server 
				 
@return			void 
**********************************************************************************************************/

void 
mqtt_packet_writer_set_keepalive_duration(app_config_struct_t*, unsigned short );


/********************************************************************************************************
@function : void set_mqtt_protocol_type(app_config_struct_t*, unsigned char )

@Description : This function is used to store the will Message if any to send in mqtt connect packet
@parameter	 :	app_config_struct_t* -> [in] -> This is address of the configuration structure used for storing 
												various parameters of the server
				unsigned char			-> [in] -> store protocol type to be send to the MQTT server the 	
												   the value is between 1 to 4				 
@return			void 
**********************************************************************************************************/

void 
mqtt_packet_writer_set_protocol_type(app_config_struct_t*, unsigned char );
/********************************************************************************************************
@function : void set_mqtt_protocol_name(app_config_struct_t*, char* )

@Description : This function is used to store the protocol name to send in mqtt connect packet
			   (This is dependent on the MQTT server used)	
@parameter	 :	app_config_struct_t* -> [in] -> This is address of the configuration structure used for storing 
												various parameters of the server
				char*			     -> [in] -> address of the protocol Name  if any to be send to the MQTT server 
				 
@return			void 
**********************************************************************************************************/

void 
mqtt_packet_writer_set_protocol_name(app_config_struct_t*, char* );

/********************************************************************************************************
@function : void set_mqtt_will_message(app_config_struct_t*, char* )

@Description : This function is used to store the will Message if any to send in mqtt connect packet
@parameter	 :	app_config_struct_t* -> [in] -> This is address of the configuration structure used for storing 
												various parameters of the server
				char*			     -> [in] -> store will Message if any to be send to the MQTT server 
				 
@return			void 
**********************************************************************************************************/

void 
mqtt_packet_writer_set_will_message(app_config_struct_t*, char* );
/********************************************************************************************************
@function : void set_mqtt_will_topic(app_config_struct_t*, char* )

@Description : This function is used to store the will Topic if any to send in mqtt connect packet
@parameter	 :	app_config_struct_t* -> [in] -> This is address of the configuration structure used for storing 
												various parameters of the server
				 char*			     -> [in] -> store will Topic if any to be send to the MQTT server 
				 
@return			void 
**********************************************************************************************************/

void 
mqtt_packet_writer_set_will_topic(app_config_struct_t*, char* );
/********************************************************************************************************
@function : void set_mqtt_client_id(app_config_struct_t*, char*)

@Description : This function is used to set the client ID of the device used for further server comm.
@parameter	 : app_config_struct_t* -> [in] -> This is address of the configuration structure used for storing 
												various parameters of the server
				char* 			    -> [in] -> pointer to client ID string to be stored in the device
					 
@return			void 
**********************************************************************************************************/

void 
mqtt_packet_writer_set_client_id(app_config_struct_t*, char* );


/********************************************************************************************************
@function : void set_mqtt_user_pass(app_config_struct_t*, const char* , const char* )

@Description : This Function sets username and password for the mqtt server used in connect packet
@parameter	 :	app_config_struct_t* -> [in] -> This is address of the configuration structure used for storing 
												various parameters of the server
				char *		  -> [in] -> address of the username to be used for the mqtt server
				char*		  -> [in] -> address of the password to be used for mqtt connect
@return			void 
**********************************************************************************************************/

void 
mqtt_packet_writer_set_user_pass(app_config_struct_t*, char* ,char* );


data_packet_t*
mqtt_packet_writer_connect(data_packet_t* dataFrame,app_config_struct_t* appConfigStruct, unsigned char connectFlag);

data_packet_t*
mqtt_packet_writer_ping(data_packet_t* dataFrame);

data_packet_t*
mqtt_packet_writer_disconn(data_packet_t* dataFrame);

data_packet_t* 
mqtt_packet_writer_unsubscribe(data_packet_t* dataFrame, char* topic, unsigned short packetID);

data_packet_t* 
mqtt_packet_writer_subscribe(data_packet_t* dataFrame, unsigned short packetID, char* subscribeTopic, app_config_struct_t* appConfigStruct);


data_packet_t*
mqtt_packet_writer_publish(data_packet_t* dataFrame, app_config_struct_t* appConfigStruct, char* publishTopic, char* dataPacket );

void mqtt_packet_writer_set_qoslevel(app_config_struct_t* appConfigStruct, QosLevel qosLevel);

void mqtt_packet_writer_set_dup_retain_flag(app_config_struct_t* appConfigStruct, int dupFlag, int retainFlag);

#endif

