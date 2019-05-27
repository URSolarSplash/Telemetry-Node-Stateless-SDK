#include "statelessTelemetryNode.h"

uint8_t
_checksum(struct Packet *p)
{
	uint8_t *p8 = (uint8_t*)p;
  uint8_t s = 0;
  for (size_t i = 0; i < 15; i++)
    s += p8[i];
  return 0xff-s;
}

uint8_t
validateChecksum(uint8_t* p)
{
  uint8_t s=0;
  for (size_t i = 0; i < 16; i++)
    s+=p[i];
  return s;
}

void
StatelessTelemetryNode::
setPacketNum(uint8_t id)
{
	switch ((DeviceID)id) {
		case DEVICE_ALLTRAX:
    case DEVICE_VESC:
    case DEVICE_MOTOR_BOARD:
    case DEVICE_THROTTLE:
			numPackets = 1;
      break;
    case DEVICE_GPS_IMU:
      numPackets = 2;
      break;
    default:
      numPackets = 1;
      break;
	}
}

void
StatelessTelemetryNode::
sendData()
{
	pack((void*)(currentPack));
	for (uint8_t i = 0; i < numPackets; i++) {
     uint8_t *outBytes = (uint8_t*)(&currentPack[i]);
     for (uint16_t j = 0; j < 16; j++) {
       _serial->write(outBytes[j]);
     }
   }
}

void
StatelessTelemetryNode::
begin(long baudrate)
{
	_serial->begin(baudrate);
}

void
StatelessTelemetryNode::
update()
{
	//read
	if(_serial->available()>0){
		rxPacket[rxIndex]=_serial->read();
		rxIndex++;
		if(rxIndex>=16){
			if(validateChecksum(rxPacket)==0xFF){
           unpack();
      }
			rxIndex=0;
		}
	}
	//write
	if(millis() - lastSent >= sendInterval){
		sendData();
		lastSent = millis();
	}
}

void
AlltraxNode::
pack(void *p)
{
  Packet* packets = (Packet*)(p);
  packets[0].startByte=0xF0;
  uint16_t* p16 = (uint16_t*) (packets[0].data);
  p16[0] = diodeTemp;
  p16[1] = inVoltage;
  p16[2] = outCurrent;
  p16[3] = inCurrent;
  uint8_t *p8 = (uint8_t*)(&p16[4]);
  p8[0] = dutyCycle;
  p8[1] = errorCode;
  p8[2] = 0x00;
  p8[3] = 0x00;
  p8[4] = 0x00;
  packets[0].metaData= (0x0F&deviceID) << 4;
  packets[0].checksum = 0x00;
  packets[0].checksum = _checksum(&packets[0]);
}

void
AlltraxNode::
unpack()
{
  throt = rxPacket[2]<<8 | rxPacket[1];
}


void
VescNode::
pack(void *p)
{
	Packet* packets = (Packet*)(p);
	packets[0].startByte=0xF0;
	uint16_t* p16 = (uint16_t*) (packets[0].data);
	p16[0] = fetTemp;
	p16[1] = inVoltage;
	p16[2] = outCurrent;
	p16[3] = inCurrent;
	uint8_t *p8 = (uint8_t*)(&p16[4]);
	p8[0] = dutyCycle;
	p8[1] = faultCode;
	p8[2] = 0x00;
	p8[3] = 0x00;
	p8[4] = 0x00;
	packets[0].metaData=(0x0F&deviceID) << 4;
	packets[0].checksum = 0x00;
	packets[0].checksum = _checksum(&packets[0]);
}

void
VescNode::
unpack()
{
	throt = rxPacket[2]<<8|rxPacket[1];
}

void
MotorBoardNode::
pack(void *p)
{
	Packet* packets = (Packet*)(p);
	packets[0].startByte=0xF0;
	uint32_t *p32 = (uint32_t*) (packets[0].data);
	uint32_t *temp = (uint32_t*) (&motorTemp);
	p32[0] = temp[0];
	p32[1] = motorRPM;
	p32[2] = propRPM;
	packets[0].metaData=(0x0F&deviceID) << 4;
	packets[0].checksum = 0x00;
	packets[0].checksum = _checksum(&packets[0]);
}

void
MotorBoardNode::
unpack(){}

void
GPSIMUNode::
pack(void *p)
{
	Packet* packets = (Packet*)(p);
	// Assemble packet 1/2
	// imu pitch, imu yaw, imu roll, num gps satellites
	packets[0].startByte=0xF0;
	uint32_t *p32_1 = (uint32_t*) (packets[0].data);
	uint32_t *imuPitch32 = (uint32_t*) (&imuPitch);
	uint32_t *imuRoll32 = (uint32_t*) (&imuRoll);
	p32_1[0] = imuPitch32[0];
	p32_1[1] = imuRoll32[0];
	uint8_t *p8_1 = (uint8_t*) (&p32_1[2]);
	p8_1[0] = numSatellites;
	p8_1[0] = fix;
	packets[0].metaData=(0x0F&deviceID) << 4;
	packets[0].checksum = _checksum(&packets[0]);

	// Assemble packet 2/2
	// latitude, longitude, speed (knots), heading
	packets[1].startByte=0xF0;
	uint32_t *p32_2 = (uint32_t*) (packets[1].data);
	uint32_t *latitude32 = (uint32_t*) (&latitude);
	uint32_t *longitude32 = (uint32_t*) (&longitude);
	uint32_t *speedKnots32 = (uint32_t*) (&speedKnots);
	p32_2[0] = latitude32[0];
	p32_2[1] = longitude32[0];
	p32_2[2] = speedKnots32[0];
	uint8_t* p8_2 = (uint8_t*)(&p32_2[3]);
	p8_2[0] = heading;
	packets[1].metaData=((0x0F&deviceID) << 4) & 0x01;
	packets[1].checksum = _checksum(&packets[1]);
}

void
GPSIMUNode::
unpack(){}

void
ThrottleNode::
pack(void *p)
{
	Packet* packets = (Packet*)(p);
	packets[0].startByte=0xF0;
	uint16_t* p16 = (uint16_t*) (packets[0].data);
	p16[0] = throt;
	p16[1] = 0x00;
	p16[2] = 0x00;
	p16[3] = 0x00;
	p16[4] = 0x00;
	p16[5] = 0x00;
	p16[6] = 0x00;
	packets[0].metaData=(0x0F&deviceID) << 4;
	packets[0].checksum = _checksum(&packets[0]);
}

void
ThrottleNode::
unpack(){}


void SolarNode::pack(void *p){
  Packet* packets = (Packet*)(p);
  packets[0].startByte = 0xF0;
  uint32_t *p32 = (uint32_t*) (packets[0].data);
  uint32_t *outCurrent1_32 = (uint32_t*) (&outCurrent1);
  uint32_t *outCurrent2_32 = (uint32_t*) (&outCurrent2);
  uint32_t *totalCurrent32 = (uint32_t*) (&totalCurrent);
  p32[0] = outCurrent1_32[0];
  p32[1] = outCurrent2_32[0];
  p32[2] = totalCurrent32[0];
  packets[0].metaData = (0x0F&deviceID) << 4;
  packets[0].checksum = _checksum(&packets[0]);
}

void
SolarNode::
unpack(){}