//===============================================================================//
//=== Deric Horn <derichorn@gmail.com>                                        ===//
//=== Moves "Worms" at different speeds, directions, and colors accross LEDs  ===//
//=== Integrates: Blynk to add worms, OTA updates, and FastLED                ===//
//===============================================================================//

#define FASTLED_INTERNAL                             //  Otherwise FastLED shows compile messages
#include <FastLED.h>
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp32.h>

typedef struct Widget {                              //  Generic data structure to handle Buttons, LEDs, etc
  int           pin;
  int           state;
  unsigned long t0;
} Widget;

typedef struct Worm {
  int           start;
  int           length;
  int           direction;
  int           speed;
  CRGB          color;
  struct Worm   *next;
} Worm;

#define     DATA_PIN            23
#define     NUM_LEDS            30
#define     kMaxSpeed           4
#define     kLogicalPositions   NUM_LEDS * kMaxSpeed          //  Logical space so fastest worms move 1 LED each turn
#define     LogicalToPhysical( logicalPosition )  logicalPosition / kMaxSpeed

const char* ssid                = "_AirHorn";
const char* password            = "Deric123";
char        auth[]              = "Xf6eWz9xieEVeZaz2lSccHRp46OtVlVS";    // You should get Auth Token in the Blynk App.  Go to the Project Settings (nut icon).

CRGB        leds[NUM_LEDS]      = {0};
Worm        *gWormsHead         = NULL;
Widget      gBuiltInLED         = {2, 0, 0};                 //  2 is built-in LED pin on esp32


void setup()
{
  pinMode( gBuiltInLED.pin, OUTPUT );

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  //  These LED's are in Green, Red, Blue order
  FastLED.setBrightness( 25 );
  (void) NewWorm( CRGB::MidnightBlue );                     //  Start one worm going

  OTASetup();
  Blynk.begin( auth, ssid, password );
}


void loop()
{
  static  long  time  = millis();
  
  ArduinoOTA.handle();
  Blynk.run();
  if ( millis() - gBuiltInLED.t0 >= 3000 )       SetWigitState( &gBuiltInLED, 1-gBuiltInLED.state );

  if ( millis() - 80 > time )
  {
    time = millis();
    PositionWorms();
    DrawWorms();
    FastLED.show();
  }

   if ( millis() - 1000*60*1 > time )      DeleteOldestWorm();    //  Every minute, delete oldest worm
}




//=====================================================================//
//============================== Worms ================================//
//=====================================================================//

Worm  *NewWorm( CRGB color )
{
  Worm  *wormP      = (Worm*) malloc( sizeof(*wormP) );
  wormP->start      = rand() % kLogicalPositions;
  wormP->length     = 1;    //1 + millis() % 3;
  wormP->direction  = 1;
  wormP->speed      = 1 + rand() % (kMaxSpeed-1);      //  Minus the minimum speed
  wormP->color      = color;
  
  wormP->next       = gWormsHead;
  gWormsHead         = wormP;
  
  return( wormP );
}

void  DeleteOldestWorm()
{
  Worm  *wormP;
  Worm  *lastWormP = NULL;
  if ( gWormsHead == NULL )  return;
  
  for ( wormP = gWormsHead ; wormP->next != NULL ; wormP = wormP->next )
    lastWormP  = wormP;
    
  if ( lastWormP != NULL )    lastWormP->next = wormP->next;
  else                        gWormsHead = wormP->next;
  free( wormP );
}


int WrappedPosition( int x )
{
  int min   = 0;
  int max   = kLogicalPositions-1;
  int i     = x;

  if ( x < min )      i = -1 * x;
  else if ( x >=max ) i = x - (x-max);

  return( i );
}

//  wormP->start is head
void  PositionWorms()
{
  for ( Worm *wormP = gWormsHead ; wormP!=NULL ; wormP=wormP->next )
  {
    int lastStart = wormP->start;

    wormP->start  = WrappedPosition( wormP->start + (wormP->speed * wormP->direction) );

    if ( wormP->start != lastStart + (wormP->speed * wormP->direction) )
      wormP->direction  = 0 - wormP->direction;
  }
}

void  DrawWorms()
{
  int     i, j;
  
  for( i=0 ; i<NUM_LEDS ; i++ )
    leds[i].fadeToBlackBy(50);
  
  for ( Worm *wormP = gWormsHead ; wormP!=NULL ; wormP=wormP->next )
  {
    for ( i=0 ; i<wormP->length ; i++ )
    {
      j = WrappedPosition( wormP->start + (i * wormP->direction) );
      j = LogicalToPhysical( j );
      leds[j] = wormP->color;
    }
  }
}


//=====================================================================//
//============================== Blynk ================================//
//=====================================================================//

BLYNK_WRITE( V1 ) 
{ 
    Serial.println("V1 Hit");
    (void) NewWorm( ColorFromPalette( RainbowColors_p, (rand() % 256) ) );
}




//=====================================================================//
//============================== Utilities ============================//
//=====================================================================//
void  SetWigitState( Widget *led, bool state )
{
  if ( led->state == state )  return;

  led->t0  = millis();
  led->state  = state;
  digitalWrite( led->pin, led->state );
}


void OTASetup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
     ArduinoOTA.setHostname("Derics_ESP32_1");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
