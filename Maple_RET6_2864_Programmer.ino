// EEPROM Programmer - code for an Arduino Mega 2560
//
// Written by K Adcock, Jan 2016.
//
// Distributed under an acknowledgement licence, because I'm
// a shallow, attention-seeking tart. :)
//
// http://www.danceswithferrets.org/geekblog/
//
// This software presents a 9600-8N1 serial port.
//
/* 
 * R[hex address]    - reads 16 bytes of data from the EEPROM
 * W[hex address]:[data in two-char hex]  - writes up to 16 bytes of data to the EEPROM
 * V     - prints the version string
 */
 
// Any data read from the EEPROM will have a CRC checksum appended to it (with a comma).
// If a string of data is sent with an optional checksum, then this will be checked
// before anything is written.
//

//yuanb:
//The following code is ported and tested on leaflabs Maple RET6
//Works for reading 2764, reading and writting of 2864
//April 02, 2017

//Ported to Arduino Studio
//Nov 25 2020

const char version_string[] = {"EEPROM Version=0.02, leaflabs RET6, Arduino IDE version"};

const char hex[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

//Connect the following EEPROM pins to named GPIO pins on RET6 board.
//Check this link for RET6 master pin map
//http://docs.leaflabs.com/static.leaflabs.com/pub/leaflabs/maple-docs/latest/hardware/maple-ret6.html#master-pin-map

/*
 * EEPROM 2864  8kx8
 * http://ww1.microchip.com/downloads/en/devicedoc/doc0001h.pdf

        ------v------
  NC    |1        28|     Vcc
  A12   |2        27|     /WE
  A7    |3        26|     NC
  A6    |4        25|     A8
  A5    |5        24|     A9
  A4    |6        23|     A11
  A3    |7        22|     /OE
  A2    |8        21|     A10
  A1    |9        20|     /CE
  A0    |10       19|     D7
  D0    |11       18|     D6
  D1    |12       17|     D5
  D2    |13       16|     D4
  GND   |14       15|     D3
        -------------
*/

#define GND     GND
#define VCC     Vin(5v)
#define PIN_A12 10
#define PIN_A11 4
#define PIN_A10 3
#define PIN_A9  2
#define PIN_A8  1
#define PIN_A7  0
#define PIN_A6  37
#define PIN_A5  36
#define PIN_A4  35
#define PIN_A3  34
#define PIN_A2  33
#define PIN_A1  32
#define PIN_A0  31

#define PIN_D7  9
#define PIN_D6  8
#define PIN_D5  7
#define PIN_D4  6
#define PIN_D3  5
#define PIN_D2  28
#define PIN_D1  29
#define PIN_D0  30

#define PIN_nWE 24
#define PIN_nOE 25
#define PIN_nCE 26

#define PIN_LED_BLUE  13

#define PIN_LED_RED    PIN_LED_BLUE
#define PIN_LED_GREEN  PIN_LED_BLUE

byte g_cmd[80]; // strings received from the controller will go in here
static const int kMaxBufferSize = 16;
byte buffer[kMaxBufferSize];

static const unsigned char vzrom_lo[8192] = {
    0xf3, 0xaf, 0x32, 0x00, 0x68, 0xc3, 0x74, 0x06, 0xc3, 0x00, 0x78, 0xe1
};

static const unsigned char vzrom_hi[8192] = {
    0x2d, 0x38, 0x02, 0x1e, 0x26, 0xc3, 0xa2, 0x19, 0x11, 0x0a, 0x00, 0xd5
};

// the setup function runs once when you press reset or power the board
void setup()
{
  Serial.begin(9600);

  //pinMode(PIN_HEARTBEAT, OUTPUT);
  //digitalWrite(PIN_HEARTBEAT, HIGH);
  
  //pinMode(PIN_LED_RED, OUTPUT);
  //digitalWrite(PIN_LED_RED, LOW);
  
  pinMode(PIN_LED_GREEN, OUTPUT);
  digitalWrite(PIN_LED_GREEN, LOW);

  // address lines are ALWAYS outputs
  pinMode(PIN_A0,  OUTPUT);
  pinMode(PIN_A1,  OUTPUT);
  pinMode(PIN_A2,  OUTPUT);
  pinMode(PIN_A3,  OUTPUT);
  pinMode(PIN_A4,  OUTPUT);
  pinMode(PIN_A5,  OUTPUT);
  pinMode(PIN_A6,  OUTPUT);
  pinMode(PIN_A7,  OUTPUT);
  pinMode(PIN_A8,  OUTPUT);
  pinMode(PIN_A9,  OUTPUT);
  pinMode(PIN_A10, OUTPUT);
  pinMode(PIN_A11, OUTPUT);
  pinMode(PIN_A12, OUTPUT);

  // control lines are ALWAYS outputs
  pinMode(PIN_nCE, OUTPUT); digitalWrite(PIN_nCE, LOW); // might as well keep the chip enabled ALL the time
  pinMode(PIN_nOE, OUTPUT); digitalWrite(PIN_nOE, HIGH);
  pinMode(PIN_nWE, OUTPUT); digitalWrite(PIN_nWE, HIGH); // not writing

  SetDataLinesAsInputs();
  SetAddress(0);
}

void SetDataLinesAsInputs()
{
  pinMode(PIN_D0, INPUT);
  pinMode(PIN_D1, INPUT);
  pinMode(PIN_D2, INPUT);
  pinMode(PIN_D3, INPUT);
  pinMode(PIN_D4, INPUT);
  pinMode(PIN_D5, INPUT);
  pinMode(PIN_D6, INPUT);
  pinMode(PIN_D7, INPUT);
}

void SetDataLinesAsOutputs()
{
  pinMode(PIN_D0, OUTPUT);
  pinMode(PIN_D1, OUTPUT);
  pinMode(PIN_D2, OUTPUT);
  pinMode(PIN_D3, OUTPUT);
  pinMode(PIN_D4, OUTPUT);
  pinMode(PIN_D5, OUTPUT);
  pinMode(PIN_D6, OUTPUT);
  pinMode(PIN_D7, OUTPUT);
}

void SetAddress(int a)
{
  digitalWrite(PIN_A0,  (a&1)?HIGH:LOW    );
  digitalWrite(PIN_A1,  (a&2)?HIGH:LOW    );
  digitalWrite(PIN_A2,  (a&4)?HIGH:LOW    );
  digitalWrite(PIN_A3,  (a&8)?HIGH:LOW    );
  digitalWrite(PIN_A4,  (a&16)?HIGH:LOW   );
  digitalWrite(PIN_A5,  (a&32)?HIGH:LOW   );
  digitalWrite(PIN_A6,  (a&64)?HIGH:LOW   );
  digitalWrite(PIN_A7,  (a&128)?HIGH:LOW  );
  digitalWrite(PIN_A8,  (a&256)?HIGH:LOW  );
  digitalWrite(PIN_A9,  (a&512)?HIGH:LOW  );
  digitalWrite(PIN_A10, (a&1024)?HIGH:LOW );
  digitalWrite(PIN_A11, (a&2048)?HIGH:LOW );
  digitalWrite(PIN_A12, (a&4096)?HIGH:LOW );
}

void SetData(byte b)
{
  digitalWrite(PIN_D0, (b&1)?HIGH:LOW  );
  digitalWrite(PIN_D1, (b&2)?HIGH:LOW  );
  digitalWrite(PIN_D2, (b&4)?HIGH:LOW  );
  digitalWrite(PIN_D3, (b&8)?HIGH:LOW  );
  digitalWrite(PIN_D4, (b&16)?HIGH:LOW );
  digitalWrite(PIN_D5, (b&32)?HIGH:LOW );
  digitalWrite(PIN_D6, (b&64)?HIGH:LOW );
  digitalWrite(PIN_D7, (b&128)?HIGH:LOW);
}

byte ReadData()
{
  byte b = 0;

  if (digitalRead(PIN_D0) == HIGH) b |= 1;
  if (digitalRead(PIN_D1) == HIGH) b |= 2;
  if (digitalRead(PIN_D2) == HIGH) b |= 4;
  if (digitalRead(PIN_D3) == HIGH) b |= 8;
  if (digitalRead(PIN_D4) == HIGH) b |= 16;
  if (digitalRead(PIN_D5) == HIGH) b |= 32;
  if (digitalRead(PIN_D6) == HIGH) b |= 64;
  if (digitalRead(PIN_D7) == HIGH) b |= 128;

  return(b);
}

// converts one character of a HEX value into its absolute value (nibble)
byte HexToVal(byte b)
{
  if (b >= '0' && b <= '9') return(b - '0');
  if (b >= 'A' && b <= 'F') return((b - 'A') + 10);
  if (b >= 'a' && b <= 'f') return((b - 'a') + 10);
  return(0);
}

void WriteVZROMToEEPROM()
{
  int addr=0;
  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_nOE, HIGH); // stop EEPROM from outputting byte
  digitalWrite(PIN_nWE, HIGH); // disables write
  SetDataLinesAsOutputs();
  delay(5);

  for (int x = 0; x < 8192; ++x)
  {
      Serial.print(hex[ (x & 0xF000) >> 12 ]);
      Serial.print(hex[ (x & 0x0F00) >> 8  ]);
      Serial.print(hex[ (x & 0x00F0) >> 4  ]);
      Serial.print(hex[ (x & 0x000F)       ]);
      Serial.println("");

    SetAddress(addr + x);
    SetData(vzrom_hi[x]);
    
    delay(10);
    digitalWrite(PIN_nWE, LOW); // enable write
    delay(10);
    digitalWrite(PIN_nWE, HIGH); // disable write
  }
  
  //digitalWrite(PIN_HEARTBEAT, LOW);
  digitalWrite(PIN_LED_RED, LOW);  
}

void WriteBufferToEEPROM(int addr, int size)
{
  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_nOE, HIGH); // stop EEPROM from outputting byte
  digitalWrite(PIN_nWE, HIGH); // disables write
  SetDataLinesAsOutputs();
  delay(5);

  for (byte x = 0; x < size; ++x)
  {
    SetAddress(addr + x);
    SetData(buffer[x]);
    
    delay(10);
    digitalWrite(PIN_nWE, LOW); // enable write
    delay(10);
    digitalWrite(PIN_nWE, HIGH); // disable write
  }
  
  //digitalWrite(PIN_HEARTBEAT, LOW);
  digitalWrite(PIN_LED_RED, LOW);
}

void ReadIntoBuffer(int addr, int size)
{
  digitalWrite(PIN_LED_GREEN, HIGH);
  
  int x;
  for (x = 0; x < size; ++x)
  {
    SetAddress(addr + x);
    delayMicroseconds(100);
    buffer[x] = ReadData();
  }
  
  digitalWrite(PIN_LED_GREEN, LOW);
}

void PrintBuffer(int size)
{
  byte chk = 0;

  for (byte x = 0; x < size; ++x)
  {
    Serial.print(hex[ (buffer[x] & 0xF0) >> 4 ]);
    Serial.print(hex[ (buffer[x] & 0x0F)      ]);
    if (x!=(size-1)) { 
      Serial.print(' ');
    }

    chk = chk ^ buffer[x];
  }

  Serial.print(", ");
  Serial.print(hex[ (chk & 0xF0) >> 4 ]);
  Serial.print(hex[ (chk & 0x0F)      ]);
  Serial.println("");
}

void ReadString()
{
  int i = 0;
  byte c;

  g_cmd[0] = 0;
  do
  {
    if (Serial.available())
    {
      c = Serial.read();
      if (c > 31 && c<91)
      {
        g_cmd[i++] = c;
        g_cmd[i] = 0;
      }
    } 
  } 
  //The serial monitor on Maple IDE can not send LF, so use ']' as line feed substitute, i.e. "V]" - display version information.
  while (c != 10 && c!= ']');
}

byte CalcBufferChecksum(byte size)
{
  byte chk = 0;

  for (byte x = 0; x < size; ++x)
  {
    chk = chk ^  buffer[x];
  }

  return(chk);
}

// the loop function runs over and over again forever
void loop()
{
  //This will program the whole 8K space in one shot.
  //WriteVZROMToEEPROM();
  
  while (true)
  {
    ReadString();
    
    if (g_cmd[0] == 'V')
    {
      Serial.println(version_string);
    }
    else if (g_cmd[0] == 'R' && g_cmd[1] != 0) // R<address>  - read kMaxBufferSize bytes from EEPROM, beginning at <address> (in hex)
    {
      int addr = 0;
      int x = 1;
      while (x < 5 && g_cmd[x] != 0)
      {
        addr = addr << 4;
        addr |= HexToVal(g_cmd[x++]);
      }
      
      digitalWrite(PIN_nWE, HIGH); // disables write
      SetDataLinesAsInputs();
      digitalWrite(PIN_nOE, LOW); // makes the EEPROM output the byte
      delay(1);
      
      ReadIntoBuffer(addr, kMaxBufferSize);

      // print address: data, checksum
      Serial.print(hex[ (addr & 0xF000) >> 12 ]);
      Serial.print(hex[ (addr & 0x0F00) >> 8  ]);
      Serial.print(hex[ (addr & 0x00F0) >> 4  ]);
      Serial.print(hex[ (addr & 0x000F)       ]);
      Serial.print(":");
      PrintBuffer(kMaxBufferSize);

      Serial.println("OK");
    }
    else if (g_cmd[0] == 'W' && g_cmd[1] != 0) // W<four byte hex address>:<data in hex, two characters per byte, max of 16 bytes per line>
    {
      int addr = 0;
      int x = 1;
      while (g_cmd[x] != ':' && g_cmd[x] != 0)
      {
        addr = addr << 4;
        addr |= HexToVal(g_cmd[x]);
        ++x;
      }

      // g_cmd[x] should now be a :
      if (g_cmd[x] == ':')
      {
        x++; // now points to beginning of data
        byte iBufferUsed = 0;
        while (g_cmd[x] && g_cmd[x+1] && iBufferUsed < kMaxBufferSize && g_cmd[x] != ',')
        {
          byte c = (HexToVal(g_cmd[x]) << 4) | HexToVal(g_cmd[x+1]);
          buffer[iBufferUsed++] = c;
          x += 2;
        }

        // if we're pointing to a comma, then the optional checksum has been provided!
        if (g_cmd[x] == ',' && g_cmd[x+1] && g_cmd[x+2])
        {
          byte checksum = (HexToVal(g_cmd[x+1]) << 4) | HexToVal(g_cmd[x+2]);

          byte our_checksum = CalcBufferChecksum(iBufferUsed);

          if (our_checksum != checksum)
          {
            // checksum fail!
            iBufferUsed = -1;
            Serial.print("ERR ");
            Serial.print(checksum, HEX);
            Serial.print(" ");
            Serial.print(our_checksum, HEX);
            Serial.println("");
          }
        }

        // buffer should now contains some data
        if (iBufferUsed > 0)
        {
          WriteBufferToEEPROM(addr, iBufferUsed);
        }

        if (iBufferUsed > -1)
        {
          Serial.println("OK");
        }
      }
    }
    else
    {
      Serial.println("ERR");
    }
  }
}
