#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "config.h"
#include "settings.h"

#define debugSerial Serial

//353 characters would be 3 digit values for every color value,
//four digits for every numeric value, which is unlikely
char wifiBuff[353];

//Set up web server
ESP8266WebServer server(80);//Port 80

//Externs
extern const char* ssid;
extern const char* pass;
extern const IPAddress apIP;
extern volatile bool stripParamsUpdated;

/******************************************************************
*******************************************************************
  Main functions
*******************************************************************
******************************************************************/

//Forward declarations
void handleRoot();
void send404();

void wifiSetup()
{
  //Ensure that we're in a reasonable state
  ESP.eraseConfig();
  WiFi.setAutoConnect(false);
  WiFi.disconnect(true);
  
  //Set up the WiFi AP
  WiFi.mode(WIFI_AP);//AP mode
  WiFi.softAPConfig(apIP,//Our IP
                    apIP,//Gateway IP
                    IPAddress(255,255,255,0));//Subnet mask
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(
  #endif
  WiFi.softAP(ssid,pass)
  #if PRINT_DEBUGGING_WIFLY
  ? F("SoftAP ready") : F("SoftAP failed"))
  #endif
  ;
  
  IPAddress myIP = WiFi.softAPIP();
  #if PRINT_DEBUGGING_WIFLY
  Serial.print(F("AP IP address: "));
  Serial.println(myIP);
  #endif
  
  //Set up the web server
  server.on("/", HTTP_GET, handleRootGet);
  server.on("/", HTTP_POST, handleRootPost);
  server.onNotFound(send404);
  server.begin();

  #if PRINT_DEBUGGING_WIFLY
  Serial.println(F("Setup Complete"));
  #endif
}

/**********************************
     Wifi Loop Procedure
**********************************/

void wifiLoop()
{
   server.handleClient();
}

void handleRootGet(){
  #if PRINT_DEBUGGING_WIFLY
  Serial.println(F("Got GET"));
  #endif
  sendIndex();
}
void handleRootPost(){
  #if PRINT_DEBUGGING_WIFLY
  Serial.println(F("Got POST"));
  #endif
  processPost();
  stripParamsUpdated = true;
  sendIndex();
}


/******************************************************************
*******************************************************************
  Utility Functions
*******************************************************************
******************************************************************/

//Goes through and sets settings when POST received
void processPost()
{
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(freeRam());
  debugSerial.println(sizeof(wifiBuff));
  debugSerial.println();
  debugSerial.println(wifiBuff);
  #endif
  
  for (int i = 0; i < server.args(); i++) {
    String key = server.argName(i);
    String valstr = server.arg(i);
    const char* val = valstr.c_str();
    debugSerial.print(key);
    debugSerial.print(": ");
    debugSerial.print(val);
    debugSerial.print("=");
    debugSerial.println(atoi(val));

    if(key.compareTo(F("color")) == 0){
      if(val[0] != 'r')
      {
        settings.patternType = PTYPE_SEQ;
        settings.LEDSeqLen = atoi(val)+1;//Number, not max index
      }
      else
      {
        settings.patternType = PTYPE_RAINBOW;
      }
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed color (seq,rbw): ("));
      debugSerial.print(settings.LEDSeqLen);
      debugSerial.print(F(","));
      debugSerial.print(settings.patternType);
      debugSerial.println(F(")"));
      #endif
    }
    else if(key.charAt(0) == 'c'){
      int cidx = atoi(key.c_str()+1);//skip the 'c' when doing atoi
      switch(key.charAt(key.length()-1)){
        case 'r':
          settings.LEDSeq[cidx] &= ~(strip.Color(255,0,0));
          settings.LEDSeq[cidx] |= strip.Color(atoi(val),0,0);
          break;
        case 'g':
          settings.LEDSeq[cidx] &= ~(strip.Color(0,255,0));
          settings.LEDSeq[cidx] |= strip.Color(0,atoi(val),0);
          break;
        case 'b':
          settings.LEDSeq[cidx] &= ~(strip.Color(0,0,255));
          settings.LEDSeq[cidx] |= strip.Color(0,0,atoi(val));
          break;
        default:
          debugSerial.print("Unknown color in key \"");
          debugSerial.print(key);
          debugSerial.println("\"");
          break;
      }
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Key \""));
      debugSerial.print(key);
      debugSerial.print(F("\" has "));
      debugSerial.print(F("cidx "));
      debugSerial.print(cidx);
      debugSerial.print(F(" and is now colored "));
      debugSerial.println(settings.LEDSeq[cidx]);
      #endif
    }
    else if(key.compareTo(F("rbw")) == 0){
      settings.rainbowWidth = atoi(val);
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed rbw: "));
      debugSerial.println(settings.rainbowWidth);
      #endif
    }
    else if(key.compareTo(F("anim")) == 0){
      settings.animMode = atoi(val+1);//prepended with 'a'
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed a: "));
      debugSerial.println(settings.animMode);
      #endif
    }
    else if(key.compareTo(F("snakeLen")) == 0){
      settings.snakeLen = atoi(val);

      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed snakeLen: "));
      debugSerial.println(settings.snakeLen);
      #endif
    }
    else if(key.compareTo(F("twinklePer")) == 0){
      settings.twinkleThresh = atoi(val);
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed twinklePer: "));
      debugSerial.println(settings.twinkleThresh);
      #endif
    }
    else if(key.compareTo(F("tran")) == 0){
      settings.transMode = atoi(val+1);//prepended with 't'
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed tran: "));
      debugSerial.println(settings.transMode);
      #endif
    }
    else if(key.compareTo(F("transT")) == 0){
      settings.transTime = atoi(val);
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed transT: "));
      debugSerial.println(settings.transTime);
      #endif
    }
    else if(key.compareTo(F("patt")) == 0){
      settings.patternMode = atoi(val+1);//prepended with 'e'
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed patt: "));
      debugSerial.println(settings.patternMode);
      #endif
    }
    else if(key.compareTo(F("dir")) == 0){
      settings.dirMode = atoi(val+1);//prepended with 'd'
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed dir: "));
      debugSerial.println(settings.dirMode);
      #endif
    }
    else if(key == F("delayT")){
      settings.delayTime = atoi(val);
      
      #if PRINT_DEBUGGING_WIFLY
      debugSerial.print(F("Processed delayT: "));
      debugSerial.println(settings.delayTime);
      #endif
    }
  }
}

/******************************
******************************
  Page Sending Utilities
******************************
******************************/

void sendChunkln(const __FlashStringHelper *str)
{
  server.sendContent(str);
  server.sendContent("\n");
}
void sendChunkln()
{
  server.sendContent("\n");
}

void sendChunk(const __FlashStringHelper *str)
{
    server.sendContent(str);
}
void sendChunk(const char *str)
{
  server.sendContent(str);
}
void sendChunk(int num)
{
  sendChunk(itoa(num,wifiBuff,10));
}

/******************************************************************
*******************************************************************
  Web Pages to Serve
*******************************************************************
******************************************************************/

/** Send a 404 error */
void send404()
{  
  //Page header
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  //Start chunked data transfer
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(404, "text/html", "");
  
  //Actual html
  sendChunkln(F("<html><head>"));
  sendChunkln(F("<title>404 Not Found (I Probably Messed Up, Sorry!)</title>"));
  sendChunkln(F("</head><body>"));
  sendChunkln(F("<h1>Not Found</h1>"));
  sendChunkln(F("<hr>"));
  sendChunkln(F("<a href=\"/\">Try Again</a>"));
  sendChunkln(F("</body></html>"));
  sendChunkln();

  //End chunked transfer
  server.sendContent(F(""));
  server.client().stop();

  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(F("404 Page Sent"));
  #endif
}

/** Send the form */
void sendIndex()
{
  //Page header
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  //Start chunked data transfer
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  
  //Actual html
  sendChunkln(F("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"));
  sendChunkln(F("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">"));
  sendChunkln(F("<head><title>LED Strip Control</title>"));
  sendChunkln(F("<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\" />"));
  sendChunkln(F("<script type=\"text/javascript\" language=\"javascript\"><!--"));
  sendChunk  (F("function updateAll(){for(var i=0; i<"));sendChunk(maxSeqLen);sendChunkln(F("; i++){updateColor(i);}}"));
  sendChunkln(F("function updateColor(idx){var target = document.getElementById(\"blk\"+idx);"));
  sendChunkln(F("var r = parseInt(document.getElementById(\"c\"+idx+\"r\").value);"));
  sendChunkln(F("var g = parseInt(document.getElementById(\"c\"+idx+\"g\").value);"));
  sendChunkln(F("var b = parseInt(document.getElementById(\"c\"+idx+\"b\").value);"));
  sendChunkln(F("target.style.backgroundColor = \"#\" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);}"));
  sendChunkln(F("--></script><style type=\"text/css\">h1{padding-top:20px;margin-bottom:0px;}"));
  sendChunkln(F(".cIn{float:left;clear:left;} .cOut{width:20px;height:20px;overflow:visible;float:left;}"));
  sendChunkln(F("input{width: 3em;}"));
  sendChunkln(F(".gridArea{"));
  sendChunkln(F("  display: grid;"));
  sendChunkln(F("  grid-template-areas: \"colorhead colorhead colorhead colorhead colorhead\""));
  sendChunkln(F("                       \" colorseq  colorseq  colorpre  colorpre  colorpre\""));
  sendChunkln(F("                       \" animhead  animhead  animhead  animhead  animhead\""));
  sendChunkln(F("                       \"animstyle  animtype animtrans   animdir  animtime\""));
  sendChunkln(F("                       \"     save      save      save      save     save\";"));
  sendChunkln(F("}"));
  sendChunkln(F("@media screen and (max-width: 600px),"));
  sendChunkln(F("       screen and (orientation:portrait){"));
  sendChunkln(F("  .gridArea{"));
  sendChunkln(F("    grid-template-areas: \"colorhead\""));
  sendChunkln(F("                         \"colorseq\""));
  sendChunkln(F("                         \"colorpre\""));
  sendChunkln(F("                         \"animhead\""));
  sendChunkln(F("                         \"animstyle\""));
  sendChunkln(F("                         \"animtype\""));
  sendChunkln(F("                         \"animtrans\""));
  sendChunkln(F("                         \"animdir\""));
  sendChunkln(F("                         \"animtime\""));
  sendChunkln(F("                         \"save\";"));
  sendChunkln(F("  }"));
  sendChunkln(F("}"));
  sendChunkln(F(".chead{grid-area: colorhead;}"));
  sendChunkln(F(".cseq{grid-area: colorseq;}"));
  sendChunkln(F(".cpre{grid-area: colorpre;}"));
  sendChunkln(F(".ahead{grid-area: animhead;}"));
  sendChunkln(F(".astyle{grid-area: animstyle;}"));
  sendChunkln(F(".atype{grid-area: animtype;}"));
  sendChunkln(F(".atrans{grid-area: animtrans;}"));
  sendChunkln(F(".adir{grid-area: animdir;}"));
  sendChunkln(F(".atime{grid-area: animtime;}"));
  sendChunkln(F(".savebuttons{grid-area: save; padding-top: 5%}"));
  sendChunkln(F("</style>"));
  sendChunkln(F("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"));
  sendChunkln(F("</head>"));
  sendChunkln(F("<body onload=\"updateAll()\"><form action=\"\" method=\"post\">"));
  sendChunkln(F("<div class=\"gridArea\"><h1 class=\"chead\">Colors</h1><div class=\"cseq\"><h2>Color Sequence (0-255)</h2>"));
  sendChunkln(F("<div class=\"cIn\"><input type=\"radio\" disabled=\"disabled\" />"));
  sendChunkln(F("<input type=\"text\" value=\"R\" disabled=\"disabled\" />"));
  sendChunkln(F("<input type=\"text\" value=\"G\" disabled=\"disabled\" />"));
  sendChunkln(F("<input type=\"text\" value=\"B\" disabled=\"disabled\" /></div>"));
  
  //Generate a spot for each color
  const char* cnames[] = {"r","g","b"};
  const int shifts[] = {16,8,0};
  char* currIdx;
  uint32_t mask;
  for(int i = 0; i < maxSeqLen; i++)
  {
    currIdx = itoa(i,wifiBuff,10);
    sendChunkln(F("<div class=\"cIn\">"));
    sendChunk(F("<input type=\"radio\" name=\"color\" value=\""));
    sendChunk(currIdx);
    if((settings.patternType == PTYPE_SEQ) && (i == settings.LEDSeqLen-1))
      sendChunkln(F("\" checked=\"checked\" />"));
    else
      sendChunkln(F("\" />"));
    
    #if PRINT_DEBUGGING_WIFLY  
    debugSerial.print(F("Sequence Color @"));
    debugSerial.print(i,DEC);
    debugSerial.print(F(": "));
    debugSerial.println(LEDSeq[i],HEX);
    #endif
      
    //Iterate over colors (RGB)
    for(int j = 0; j < 3; j++)
    {
      sendChunk(F("<input type=\"number\" name=\"c"));
      sendChunk(currIdx);sendChunk(cnames[j]);sendChunk(F("\" id=\"c"));
      sendChunk(currIdx);sendChunk(cnames[j]);sendChunk(F("\" value=\""));
      mask = 0xFF;
      mask = mask<<shifts[j];
      sendChunk(itoa((settings.LEDSeq[i]&mask)>>shifts[j],wifiBuff+6,10));
      sendChunk(F("\" min=\"0\" max=\"255\" step=\"1\" onblur=\"updateColor("));
      sendChunk(currIdx);sendChunkln(F(")\"/>"));
    }
    sendChunk(F("</div><div id=\"blk"));
    sendChunk(currIdx);sendChunkln(F("\" class=\"cOut\"></div><br/>"));
  }

  sendChunkln(F("</div><div class=\"cpre\"><h2>Presets</h2>"));
  sendChunk  (F("<input type=\"radio\" name=\"color\" value=\"r\""));if(settings.patternType==PTYPE_RAINBOW){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Rainbow "));
  sendChunk  (F("<input type=\"number\" name=\"rbw\" value=\""));sendChunk(settings.rainbowWidth);sendChunk(F("\" min=\"1\" max=\""));sendChunk(768);sendChunkln(F("\" step=\"1\" />Width<br/>"));
    /*<input type="radio" name="color" value="p0" />Holiday<br/>
      <input type="radio" name="color" value="p1" />Patriotic<br/>
      <input type="radio" name="color" value="p2" />Halloween<br/>
      <input type="radio" name="color" value="p3" />Valentine's<br/>
      <input type="radio" name="color" value="p4" />Sunset<br/>
      <input type="radio" name="color" value="p5" />Winter<br/>
      <input type="radio" name="color" value="p6" />Jungle<br/>*/
  sendChunkln(F("</div>"));
  
  //Animation styles
  sendChunkln(F("<h1 class=\"ahead\">Animation</h1><div class=\"astyle\"><h2>Style</h2>"));
  sendChunk  (F("<input type=\"radio\" name=\"anim\" value=\"a0\""));if(settings.animMode==ANIM_NONE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />None<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"anim\" value=\"a1\""));if(settings.animMode==ANIM_SCROLL){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Scroll<br/>"));
  sendChunk  (F("<input type=\"radio\" name=\"anim\" value=\"a2\""));if(settings.animMode==ANIM_SNAKE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Snake "));
  sendChunk  (F("<input type=\"number\" name=\"snakeLen\" value=\""));sendChunk(settings.snakeLen);sendChunk(F("\" min=\"1\" max=\""));sendChunk(numLEDs);sendChunkln(F("\" step=\"1\" /> long<br/>"));
  sendChunk  (F("<input type=\"radio\" name=\"anim\" value=\"a3\""));if(settings.animMode==ANIM_TWINKLE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Twinkle "));
  sendChunk  (F("<input type=\"number\" name=\"twinklePer\" value=\""));sendChunk(settings.twinkleThresh);sendChunkln(F("\" min=\"0\" max=\"100\" step=\"1\" />% on<br/></div>"));
  
  sendChunkln(F("<div class=\"atype\"><h2>Type</h2>"));
  sendChunkln(F("<input type=\"radio\" name=\"patt\" value=\"e0\""));if(settings.patternMode==PATTERN_SERIAL){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Serial<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"patt\" value=\"e1\""));if(settings.patternMode==PATTERN_BLOCKS){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Blocks<br/>"));      
  sendChunkln(F("<input type=\"radio\" name=\"patt\" value=\"e2\""));if(settings.patternMode==PATTERN_SOLIDS){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Solids<br/></div>"));
  
  sendChunkln(F("<div class=\"atrans\"><h2>Transition</h2>"));
  sendChunkln(F("<input type=\"radio\" name=\"tran\" value=\"t0\""));if(settings.transMode==TRANS_NONE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />None<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"tran\" value=\"t1\""));if(settings.transMode==TRANS_PULSE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Pulse<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"tran\" value=\"t2\""));if(settings.transMode==TRANS_FLASH){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Flash<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"tran\" value=\"t3\""));if(settings.transMode==TRANS_FADE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Fade<br/></div>"));
  
  sendChunkln(F("<div class=\"adir\"><h2>Direction</h2>"));
  sendChunkln(F("<input type=\"radio\" name=\"dir\" value=\"d0\""));if(settings.dirMode==DIR_L){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Left<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"dir\" value=\"d1\""));if(settings.dirMode==DIR_R){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Right<br/>"));
  /*sendChunkln(F("<input type=\"radio\" name=\"dir\" value=\"d2\""));if(animMode==DIR_OUT){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Out<br/>"));
    sendChunkln(F("<input type=\"radio\" name=\"dir\" value=\"d3\""));if(animMode==DIR_IN){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />In<br/>"))*/
  sendChunkln(F("</div>"));
  
  sendChunkln(F("<div class=\"atime\"><h2>Timing</h2>"));
  sendChunk  (F("<input type=\"number\" name=\"transT\" style=\"width:5em;\" value=\""));sendChunk(settings.transTime);sendChunkln(F("\" min=\"0\" step=\"1\" /> Transition (ms)<br/>"));
  sendChunk  (F("<input type=\"number\" name=\"delayT\" style=\"width:5em;\" value=\""));sendChunk(settings.delayTime);sendChunkln(F("\" min=\"0\" step=\"1\" /> Delay (ms)<br/></div>"));
  sendChunkln(F("</div>"));
  sendChunkln(F("<div class=\"savebuttons\">"));
  /*sendChunkln(F("<input type=\"checkbox\" name=\"eeprom\" />Persist on reboot<br/>"));*/
  sendChunkln(F("<input type=\"submit\" value=\"Save Settings\" style=\"padding:10px;width:auto;\"/>"));
  sendChunkln(F("</div></form></body></html>"));
  sendChunkln();

  //End chunked transfer
  server.sendContent(F(""));
  server.client().stop();
  
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(F("Index Page Sent"));
  #endif
}
