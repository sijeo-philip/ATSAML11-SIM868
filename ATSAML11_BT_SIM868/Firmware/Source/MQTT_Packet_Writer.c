#include "MQTT_Packet_Writer.h"
#include <stdlib.h>
#include <string.h>


static int data_frame_init(data_packet_t* dataFrame)
{
  dataFrame->totalSize = 0;
  dataFrame->nextPos = 0;
  memset(dataFrame->data, '\0', sizeof(dataFrame->data));
  return 0;
}


data_packet_t*
mqtt_packet_writer_connect(data_packet_t* dataFrame,app_config_struct_t* appConfigStruct, unsigned char connectFlag)
{
			unsigned int tempStringSize=0;
	    dataFrame->data[0] = 0x10;
      dataFrame->nextPos = 2;
      tempStringSize = strlen(appConfigStruct->protocolName);
      dataFrame->data[dataFrame->nextPos++] = (tempStringSize >> 8) & 0xFF;
      dataFrame->data[dataFrame->nextPos++] = tempStringSize & 0xFF;
      strncpy(&dataFrame->data[dataFrame->nextPos],appConfigStruct->protocolName, tempStringSize);
      dataFrame->nextPos += tempStringSize;
      dataFrame->data[dataFrame->nextPos++] = appConfigStruct->protocolType;
      dataFrame->data[dataFrame->nextPos++] = connectFlag;
      dataFrame->data[dataFrame->nextPos++] = ((appConfigStruct->keepAliveDurationSec) >> 8) & 0xFF;
      dataFrame->data[dataFrame->nextPos++] =  (appConfigStruct->keepAliveDurationSec) & 0xFF;
      
			// Writing Client ID to the MQTT PACKET
      dataFrame->data[dataFrame->nextPos++] = (((short int)strlen(appConfigStruct->clientID) >> 8) & 0XFF);
      dataFrame->data[dataFrame->nextPos++] = (strlen(appConfigStruct->clientID) & 0xFF);
      tempStringSize = strlen(appConfigStruct->clientID);
      strncpy(&dataFrame->data[dataFrame->nextPos], appConfigStruct->clientID,tempStringSize);
      dataFrame->nextPos += tempStringSize;

      // Writing WILL TOPIC to the MQTT Packet
      if (connectFlag & WILL_FLAG)
	   {
			dataFrame->data[dataFrame->nextPos++] = (((short int)strlen(appConfigStruct->willTopic) >> 8) & 0xFF);
	    dataFrame->data[dataFrame->nextPos++] = (strlen(appConfigStruct->willTopic) & 0xFF);
	    tempStringSize = strlen(appConfigStruct->willTopic);
			strncpy(&dataFrame->data[dataFrame->nextPos],appConfigStruct->willTopic, tempStringSize);
	    dataFrame->nextPos += tempStringSize;
			}
      // Writing WILL MESSAGE to the MQTT PACKET
      if (connectFlag & WILL_RETAIN)
			{
				dataFrame->data[dataFrame->nextPos++] = (((short int)strlen(appConfigStruct->willMessage) >> 8) & 0XFF);
	  	  dataFrame->data[dataFrame->nextPos++] = (strlen(appConfigStruct->willMessage) & 0XFF);
	  	  tempStringSize = strlen(appConfigStruct->willMessage);
				strncpy(&dataFrame->data[dataFrame->nextPos],  appConfigStruct->willMessage, tempStringSize);
	  	  dataFrame->nextPos += tempStringSize;
			}
      // Writing UserName to the MQTT PACKET
      if (connectFlag & USERNAME_FLAG)
			{
	      dataFrame->data[dataFrame->nextPos++] = (((short int)strlen(appConfigStruct->MqttUsername) >> 8) & 0XFF);
	      dataFrame->data[dataFrame->nextPos++] = (strlen(appConfigStruct->MqttUsername) & 0xFF);
	      tempStringSize = strlen(appConfigStruct->MqttUsername);
				strncpy(&dataFrame->data[dataFrame->nextPos], appConfigStruct->MqttUsername, tempStringSize);
	  	  dataFrame->nextPos += tempStringSize;
	     }
      // Writing Password to the MQTT PACKET
      if (connectFlag & PASSWORD_FLAG)
			{
				dataFrame->data[dataFrame->nextPos++] = (((short int)strlen(appConfigStruct->MqttPassword) >> 8) & 0XFF);
	  	  dataFrame->data[dataFrame->nextPos++] = (strlen(appConfigStruct->MqttPassword) & 0xFF);
	  	  tempStringSize = strlen(appConfigStruct->MqttPassword);
				strncpy(&dataFrame->data[dataFrame->nextPos], appConfigStruct->MqttPassword, tempStringSize);
	  	  dataFrame->nextPos += tempStringSize;
			}

      // Forming the complete data Frame for MQTT PACKET
      dataFrame->data[1] = (unsigned char)(dataFrame->nextPos - 2) & 0xFF;
      dataFrame->totalSize = dataFrame->nextPos + 1;
      dataFrame->packetType = MQTT_CONN_PACKET;
      return dataFrame;
 
}

data_packet_t*
mqtt_packet_writer_ping(data_packet_t* dataFrame)
{
	    dataFrame->data[dataFrame->nextPos++] = 0xC0;
      dataFrame->data[dataFrame->nextPos] = 0x00;
      dataFrame->totalSize = 2;
			dataFrame->packetType = MQTT_PING_PACKET;
     return dataFrame;
}

data_packet_t*
mqtt_packet_writer_disconn(data_packet_t* dataFrame){
	    dataFrame->data[dataFrame->nextPos++] = 0xE0;
      dataFrame->data[dataFrame->nextPos] = 0x00;
      dataFrame->totalSize = 2;
			dataFrame->packetType=MQTT_DISCONN_PACKET;
      return dataFrame;
}

data_packet_t* 
mqtt_packet_writer_unsubscribe(data_packet_t* dataFrame, char* topic, unsigned short packetID){
	   unsigned int tempStringSize=0;
	    dataFrame->data[0] = 0xA2;
      dataFrame->nextPos = 2;

      // Writing packet ID for the packet
      dataFrame->data[dataFrame->nextPos++] = (((short int)packetID >> 8) & 0xFF);
      dataFrame->data[dataFrame->nextPos++] = packetID & 0xFF;

      // Writing lenght of the unsubcribe topic
      dataFrame->data[dataFrame->nextPos++] = (((short int)strlen(topic) >> 8) & 0xFF);
      dataFrame->data[dataFrame->nextPos++] = (strlen(topic) & 0xFF);

      // Writing Topic to the Frame
      tempStringSize = strlen(topic);
      strncpy(&dataFrame->data[dataFrame->nextPos],topic, tempStringSize);
      dataFrame->nextPos += tempStringSize;

      dataFrame->data[1] = (dataFrame->nextPos - 2) & 0xFF;
      dataFrame->totalSize = dataFrame->nextPos;
       return dataFrame;
}

data_packet_t* 
mqtt_packet_writer_subscribe(data_packet_t* dataFrame, unsigned short packetID, char* subscribeTopic, app_config_struct_t* appConfigStruct){
	unsigned int tempStringSize=0;
	unsigned char Temp = 0;
      dataFrame->data[0] = 0x82;
      dataFrame->nextPos = 2;

      // Writing Packet ID to the MQTT Packet
      dataFrame->data[dataFrame->nextPos++] = (((short int)packetID >> 8) & 0XFF);
      dataFrame->data[dataFrame->nextPos++] = packetID & 0xFF;

      // Writing length of Topic to the MQTT Packet
      dataFrame->data[dataFrame->nextPos++] = (((short int)strlen(subscribeTopic) >> 8) & 0xFF);
      dataFrame->data[dataFrame->nextPos++] = (strlen(subscribeTopic) & 0xFF);

      // Write Subscribe Topic to the MQTT Packet
      tempStringSize = strlen(subscribeTopic);
      strncpy(&dataFrame->data[dataFrame->nextPos],subscribeTopic, tempStringSize);
      dataFrame->nextPos += tempStringSize;

      if (appConfigStruct->qosLevel == QosLevel1)
					Temp = Temp | SUBQoSLEVEL0;
      else if (appConfigStruct->qosLevel == QosLevel2)
					Temp = Temp | SUBQoSLEVEL1;

      // qosLevel written if the qosLevel is greater than 0
      dataFrame->data[dataFrame->nextPos] = Temp;

      dataFrame->data[1] = (dataFrame->nextPos - 2) & 0xFF;

      dataFrame->totalSize = dataFrame->nextPos + 1;
      return dataFrame;
   
}


data_packet_t*
mqtt_packet_writer_publish(data_packet_t* dataFrame, app_config_struct_t* appConfigStruct, char* publishTopic, char* dataPacket ){
			unsigned int tempStringSize=0;
      unsigned char Temp = 0x30;
      dataFrame->nextPos = 2;

      if (appConfigStruct->qosLevel == QosLevel1)
	Temp = Temp | QosLevel0;
      else if (appConfigStruct->qosLevel == QosLevel2)
	Temp = Temp | QosLevel1;

      // Check Retain Bit
      if (appConfigStruct->retain == 1) Temp = Temp | RETAIN;
      // Check DUP bit
      if (appConfigStruct->dup == 1) Temp = Temp | DUP;
      // Publis Topic is written to the Packet
      dataFrame->data[dataFrame->nextPos++] = (((short int)strlen(publishTopic) >> 8) & 0xFF);
      dataFrame->data[dataFrame->nextPos++] = (strlen(publishTopic) & 0xFF);
      tempStringSize = strlen(publishTopic);
      strncpy(&dataFrame->data[dataFrame->nextPos],publishTopic, tempStringSize);
      dataFrame->nextPos += tempStringSize;

      // qosLevel written to the packet
      if (appConfigStruct->qosLevel == QosLevel1 ||
	  appConfigStruct->qosLevel == QosLevel2)
	{
	  dataFrame->data[dataFrame->nextPos++] = 0x00;
	  dataFrame->data[dataFrame->nextPos++] = 0x0A;
	}

      // Publish Data is written to the Packet
      tempStringSize = strlen(dataPacket);
      strncpy(&dataFrame->data[dataFrame->nextPos], dataPacket, tempStringSize);
      dataFrame->nextPos += tempStringSize;

      if (((dataFrame->nextPos - 1) >= 0) && ((dataFrame->nextPos - 1) <= 255))
	{
	  dataFrame->data[1] = (dataFrame->nextPos - 2) & 0xFF;
	}
      else
	{
	  return NULL;
	}
      dataFrame->totalSize = dataFrame->nextPos + 1;
      return dataFrame;
 
}
//****************************************** MQTT CONFIGURATION SETTINGS SET FUNCTIONS******************************
void mqtt_packet_writer_set_user_pass(app_config_struct_t* appConfigStruct, char* userName,char* passWord)
{
  unsigned int tempStringSize = 0;
  tempStringSize = strlen(userName);
  strncpy(appConfigStruct->MqttUsername, userName, tempStringSize);
  tempStringSize = strlen(passWord);
  strncpy(appConfigStruct->MqttPassword, passWord, tempStringSize);
}


void mqtt_packet_writer_set_client_id(app_config_struct_t* appConfigStruct, char* clientID)
{
  size_t tempStringSize;
  tempStringSize = strlen(clientID);
  strncpy(appConfigStruct->clientID, clientID, tempStringSize);
  
}

void mqtt_packet_writer_set_will_topic(app_config_struct_t* appConfigStruct, char* willTopic)
{
  size_t tempStringSize;
  tempStringSize = strlen(willTopic);
  strncpy(appConfigStruct->willTopic, willTopic, tempStringSize);
}

void mqtt_packet_writer_set_will_message(app_config_struct_t* appConfigStruct,char* willMessage)
{
  size_t tempStringSize;
  tempStringSize = strlen(willMessage);
  strncpy(appConfigStruct->willMessage, willMessage, tempStringSize);
}

void mqtt_packet_writer_set_protocol_name(app_config_struct_t* appConfigStruct,
			    char* protocolName)
{
  size_t tempStringSize;
  tempStringSize = strlen(protocolName);
  strncpy(appConfigStruct->protocolName, protocolName, tempStringSize);
 }

void mqtt_packet_writer_set_protocol_type(app_config_struct_t* appConfigStruct,
			    unsigned char protocolType)
{
  appConfigStruct->protocolType = protocolType;
}


void mqtt_packet_writer_set_keepalive_duration(app_config_struct_t* appConfigStruct,
			    unsigned short keepAliveDuration)
{
  appConfigStruct->keepAliveDurationSec = keepAliveDuration;
}

void mqtt_packet_writer_set_qoslevel(app_config_struct_t* appConfigStruct, QosLevel qosLevel)
{
  appConfigStruct->qosLevel = qosLevel;
  
}

void mqtt_packet_writer_set_dup_retain_flag(app_config_struct_t* appConfigStruct, int dupFlag,
			 int retainFlag)
{
  appConfigStruct->dup = dupFlag;
  appConfigStruct->retain = retainFlag;
}



