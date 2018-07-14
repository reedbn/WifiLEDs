#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

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
  WiFi.setAutoConnect(false);
  WiFi.disconnect(true);
  
  //Set up the WiFi AP
  WiFi.mode(WIFI_AP);//AP mode
  WiFi.softAPConfig(apIP,//Our IP
                    apIP,//Gateway IP
                    IPAddress(255,255,255,0));//Subnet mask
  WiFi.softAP(ssid,pass);
  IPAddress myIP = WiFi.softAPIP();
  #if PRINT_DEBUGGING_WIFLY
  Serial.print(F("AP IP address: "));
  Serial.println(myIP);
  
  //Set up the web server
  server.on("/", HTTP_GET, handleRootGet);
  server.on("/", HTTP_POST, handleRootPost);
  server.onNotFound(send404);
  server.begin();
  #endif
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
  String message;
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  Serial.println(message);
  return;
  
  //Skip to \r\n\r\n, since that separates the
  //header from the data
  Serial.find("\r\n\r\n",4);
  
  //Grab everything at once
  memset(wifiBuff,0,sizeof(wifiBuff));
  Serial.readBytes(wifiBuff,sizeof(wifiBuff));
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(freeRam());
  debugSerial.println(sizeof(wifiBuff));
  debugSerial.println();
  debugSerial.println(wifiBuff);
  #endif
  
  //Big clunky switch statement to handle actual parsing
  int idx = 0;
  while((idx < sizeof(wifiBuff)) && (wifiBuff[idx] != 0))
  {
    switch(wifiBuff[idx++])
    {
      case 'c':
      {
        if(wifiBuff[idx] == 'o')
        {
          idx = findIdx(wifiBuff,idx,'=',sizeof(wifiBuff));
          if(wifiBuff[idx] != 'r')
          {
            patternType = PTYPE_SEQ;
            LEDSeqLen = readInt(wifiBuff,idx++,sizeof(wifiBuff))+1;//Number, not max index
          }
          else
          {
            patternType = PTYPE_RAINBOW;
          }
          
          #if PRINT_DEBUGGING_WIFLY
          debugSerial.print(F("Processed co (seq,rbw): ("));
          debugSerial.print(LEDSeqLen);
          debugSerial.print(F(","));
          debugSerial.print(patternType);
          debugSerial.println(F(")"));
          #endif
        }
        else
        {
          int cidx = readInt(wifiBuff,idx,sizeof(wifiBuff));
          idx = findIdx(wifiBuff,idx,'=',sizeof(wifiBuff));
          int r = readInt(wifiBuff,idx,sizeof(wifiBuff));
          idx = findIdx(wifiBuff,idx,'=',sizeof(wifiBuff));
          int g = readInt(wifiBuff,idx,sizeof(wifiBuff));
          idx = findIdx(wifiBuff,idx,'=',sizeof(wifiBuff));
          int b = readInt(wifiBuff,idx,sizeof(wifiBuff));/**/
          
          LEDSeq[cidx] = strip.Color(r,g,b);
          
          #if PRINT_DEBUGGING_WIFLY
          debugSerial.print(F("Processed c (cidx,r,g,b): ("));
          debugSerial.print(cidx);
          debugSerial.print(",");
          debugSerial.print(r);
          debugSerial.print(",");
          debugSerial.print(g);
          debugSerial.print(",");
          debugSerial.print(b);
          debugSerial.println(")");
          #endif
        }
        break;          
      }
      case 'r':
      {
        rainbowWidth = readInt(wifiBuff,idx,sizeof(wifiBuff));
        
        #if PRINT_DEBUGGING_WIFLY
        debugSerial.print(F("Processed r: "));
        debugSerial.println(rainbowWidth);
        #endif
        break;
      }
      case 'a':
      {
        animMode = readInt(wifiBuff,idx,sizeof(wifiBuff));
        
        #if PRINT_DEBUGGING_WIFLY
        debugSerial.print(F("Processed a: "));
        debugSerial.println(animMode);
        #endif
        break;
      }
      case 't':
      {
        if(wifiBuff[idx] == 'w')
        {
          twinkleThresh = readInt(wifiBuff,idx,sizeof(wifiBuff));
          
          #if PRINT_DEBUGGING_WIFLY
          debugSerial.print(F("Processed tw: "));
          debugSerial.println(twinkleThresh);
          #endif
        }
        else if(wifiBuff[idx+3] == '=')
        {
          transMode = readInt(wifiBuff,idx,sizeof(wifiBuff));
          
          #if PRINT_DEBUGGING_WIFLY
          debugSerial.print(F("Processed t***=: "));
          debugSerial.println(transMode);
          #endif
        }
        else
        {
          transTime = readInt(wifiBuff,idx,sizeof(wifiBuff));
          
          #if PRINT_DEBUGGING_WIFLY
          debugSerial.print(F("Processed t: "));
          debugSerial.println(transTime);
          #endif
        }
        break;
      }
      case 'p':
      {
        patternMode = readInt(wifiBuff,idx,sizeof(wifiBuff));
        
        #if PRINT_DEBUGGING_WIFLY
        debugSerial.print(F("Processed p: "));
        debugSerial.println(patternMode);
        #endif
        break;
      }
      case 'd':
      {
        if(wifiBuff[idx] == 'i')
        {
          dirMode = readInt(wifiBuff,idx,sizeof(wifiBuff));
          
          #if PRINT_DEBUGGING_WIFLY
          debugSerial.print(F("Processed di: "));
          debugSerial.println(dirMode);
          #endif
        }
        else
        {
          delayTime = readInt(wifiBuff,idx,sizeof(wifiBuff));
          
          #if PRINT_DEBUGGING_WIFLY
          debugSerial.print(F("Processed d: "));
          debugSerial.println(delayTime);
          #endif
        }
        break;
      }
      case '&':
      {
        #if PRINT_DEBUGGING_WIFLY
        debugSerial.println(F("Skipped &"));
        #endif
        break;
      }
      default:
      {
        #if PRINT_DEBUGGING_WIFLY
        debugSerial.println(F("Skipped char"));
        #endif
        break;
      }
    }
    idx = findIdx(wifiBuff,idx,'&',sizeof(wifiBuff));
  }
}

/******************************
******************************
  Parsing Utilities
******************************
******************************/

int readInt(char wifiBuff[],int startIdx,int maxIdx)
{
  #if PRINT_DEBUGGING_WIFLY_DETAIL
  debugSerial.print(F("readInt startIdx,arrsize,val: "));
  debugSerial.print(startIdx);
  debugSerial.print(F(","));
  debugSerial.print(sizeof(wifiBuff));
  debugSerial.print(F(","));
  debugSerial.println((int)(&wifiBuff));/**/
  #endif
  
  char myBuff[10];//I don't think we should get more than 10 digits
  memset(myBuff,0,sizeof(myBuff));
  int myIdx = 0;
  boolean set = false;
  for(int i=startIdx;i < maxIdx;i++)
  {
    #if PRINT_DEBUGGING_WIFLY_DETAIL
    debugSerial.print(F("idx:val:myBuff - "));
    debugSerial.print(i);
    debugSerial.print(F(":"));
    debugSerial.print(wifiBuff[i]);
    debugSerial.print(F(":"));
    debugSerial.println(myBuff);/**/
    #endif
    
    if(isDigit(wifiBuff[i]))
    {
      myBuff[myIdx++] = wifiBuff[i];
      set = true;
    }
    else if(set)
    {
      return atoi(myBuff);
    }
  }
  return -1;
}

int findIdx(char wifiBuff[],int startIdx,char ch,int maxIdx)
{
  #if PRINT_DEBUGGING_WIFLY_DETAIL
  debugSerial.print(F("findInt startIdx,arrsize,val: "));
  debugSerial.print(startIdx);
  debugSerial.print(F(","));
  debugSerial.print(sizeof(wifiBuff));
  debugSerial.print(F(","));
  debugSerial.println((int)(&wifiBuff));/**/
  #endif
  
  for(int i=startIdx;i<maxIdx;i++)
  {
    #if PRINT_DEBUGGING_WIFLY_DETAIL
    debugSerial.print(F("    idx:val - "));
    debugSerial.print(i);
    debugSerial.print(F(":"));
    debugSerial.println(wifiBuff[i]);/**/
    #endif
    if(wifiBuff[i] == ch)
    {
      return i+1;
    }
  }
  return -1;
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
  sendChunkln(F("var r = parseInt(document.getElementById(\"c\"+idx+\"r\").value)*2;"));
  sendChunkln(F("var g = parseInt(document.getElementById(\"c\"+idx+\"g\").value)*2;"));
  sendChunkln(F("var b = parseInt(document.getElementById(\"c\"+idx+\"b\").value)*2;"));
  sendChunkln(F("target.style.backgroundColor = \"#\" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);}"));
  sendChunkln(F("--></script><style type=\"text/css\">h1{padding-top:20px;margin-bottom:0px;}"));
  sendChunkln(F(".cIn{float:left;clear:left;} .cOut{width:20px;height:20px;overflow:visible;float:left;}"));
  sendChunkln(F(".section{float:left;margin-right:50px;}</style></head>"));
  sendChunkln(F("<body onload=\"updateAll()\"><form action=\"\" method=\"post\"><h1>Colors</h1>"));
  sendChunkln(F("<div><div class=\"section\"><h2>Color Sequence (0-127)</h2>"));
  sendChunkln(F("<div class=\"cIn\"><input type=\"radio\" disabled=\"disabled\" />"));
  sendChunkln(F("<input type=\"text\" value=\"R\" size=\"3\" disabled=\"disabled\" />"));
  sendChunkln(F("<input type=\"text\" value=\"G\" size=\"3\" disabled=\"disabled\" />"));
  sendChunkln(F("<input type=\"text\" value=\"B\" size=\"3\" disabled=\"disabled\" /></div>"));
  
  //Generate a spot for each color
  const char* cnames[] = {"r","g","b"};
  const int shifts[] = {8,16,0};
  char* currIdx;
  uint32_t mask;
  for(int i = 0; i < maxSeqLen; i++)
  {
    currIdx = itoa(i,wifiBuff,10);
    sendChunkln(F("<div class=\"cIn\">"));
    sendChunk(F("<input type=\"radio\" name=\"color\" value=\""));
    sendChunk(currIdx);
    if((patternType == PTYPE_SEQ) && (i == LEDSeqLen-1))
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
      sendChunk(F("<input type=\"text\" name=\"c"));
      sendChunk(currIdx);sendChunk(cnames[j]);sendChunk(F("\" id=\"c"));
      sendChunk(currIdx);sendChunk(cnames[j]);sendChunk(F("\" value=\""));
      mask = 0x7F;
      mask = mask<<shifts[j];
      sendChunk(itoa((LEDSeq[i]&mask)>>shifts[j],wifiBuff+6,10));
      sendChunk(F("\" size=\"3\" onblur=\"updateColor("));
      sendChunk(currIdx);sendChunkln(F(")\"/>"));
    }
    sendChunk(F("</div><div id=\"blk"));
    sendChunk(currIdx);sendChunkln(F("\" class=\"cOut\"></div><br/>"));
  }

  sendChunkln(F("</div><div class=\"section\"><h2>Presets</h2>"));
  sendChunk  (F("<input type=\"radio\" name=\"color\" value=\"r\""));if(patternType==PTYPE_RAINBOW){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Rainbow "));
  sendChunk  (F("<input type=\"text\" name=\"rbw\" value=\""));sendChunk(rainbowWidth);sendChunkln(F("\" size=\"5\" />Width<br/>"));
    /*<input type="radio" name="color" value="p0" />Holiday<br/>
      <input type="radio" name="color" value="p1" />Patriotic<br/>
      <input type="radio" name="color" value="p2" />Halloween<br/>
      <input type="radio" name="color" value="p3" />Valentine's<br/>
      <input type="radio" name="color" value="p4" />Sunset<br/>
      <input type="radio" name="color" value="p5" />Winter<br/>
      <input type="radio" name="color" value="p6" />Jungle<br/>*/
  sendChunkln(F("</div></div>"));
  
  //Animation styles
  sendChunkln(F("<h1 style=\"clear:both;\">Animation</h1><div><div class=\"section\"><h2>Style</h2>"));
  sendChunk  (F("<input type=\"radio\" name=\"anim\" value=\"a0\""));if(animMode==ANIM_NONE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />None<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"anim\" value=\"a1\""));if(animMode==ANIM_SCROLL){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Scroll<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"anim\" value=\"a2\""));if(animMode==ANIM_SNAKE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Snake<br/>"));
  sendChunk  (F("<input type=\"radio\" name=\"anim\" value=\"a3\""));if(animMode==ANIM_TWINKLE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Twinkle "));
  sendChunk  (F("<input type=\"text\" name=\"twinklePer\" value=\""));sendChunk(twinkleThresh);sendChunkln(F("\" size=\"3\" />% on<br/></div>"));
  
  sendChunkln(F("<div class=\"section\"><h2>Type</h2>"));
  sendChunkln(F("<input type=\"radio\" name=\"patt\" value=\"e0\""));if(patternMode==PATTERN_SERIAL){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Serial<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"patt\" value=\"e1\""));if(patternMode==PATTERN_BLOCKS){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Blocks<br/>"));      
  sendChunkln(F("<input type=\"radio\" name=\"patt\" value=\"e2\""));if(patternMode==PATTERN_SOLIDS){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Solids<br/></div>"));
  
  sendChunkln(F("<div class=\"section\"><h2>Transition</h2>"));
  sendChunkln(F("<input type=\"radio\" name=\"tran\" value=\"t0\""));if(transMode==TRANS_NONE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />None<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"tran\" value=\"t1\""));if(transMode==TRANS_PULSE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Pulse<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"tran\" value=\"t2\""));if(transMode==TRANS_FLASH){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Flash<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"tran\" value=\"t3\""));if(transMode==TRANS_FADE){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Fade<br/></div>"));
  
  sendChunkln(F("<div class=\"section\"><h2>Direction</h2>"));
  sendChunkln(F("<input type=\"radio\" name=\"dir\" value=\"d0\""));if(dirMode==DIR_L){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Left<br/>"));
  sendChunkln(F("<input type=\"radio\" name=\"dir\" value=\"d1\""));if(dirMode==DIR_R){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Right<br/>"));
  /*sendChunkln(F("<input type=\"radio\" name=\"dir\" value=\"d2\""));if(animMode==DIR_OUT){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />Out<br/>"));
    sendChunkln(F("<input type=\"radio\" name=\"dir\" value=\"d3\""));if(animMode==DIR_IN){sendChunk(F(" checked=\"checked\" "));};sendChunkln(F(" />In<br/>"))*/
  sendChunkln(F("</div>"));
  
  sendChunkln(F("<div class=\"section\"><h2>Timing</h2>"));
  sendChunk  (F("<input type=\"text\" name=\"transT\" value=\""));sendChunk(transTime);sendChunkln(F("\" size=\"3\" /> Transition (ms)<br/>"));
  sendChunk  (F("<input type=\"text\" name=\"delayT\" value=\""));sendChunk(delayTime);sendChunkln(F("\" size=\"3\" /> Delay (ms)<br/></div>"));
  
  sendChunkln(F("</div><div style=\"clear:both;padding-top:30px;\">"));
    /*<input type="checkbox" name="eeprom" />Persist on reboot<br/>*/
  sendChunkln(F("<input type=\"submit\" value=\"Save Settings\" style=\"padding:10px;\"/>"));
  sendChunkln(F("</div></form></body></html>"));
  sendChunkln();

  //End chunked transfer
  server.sendContent(F(""));
  server.client().stop();
  
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(F("Index Page Sent"));
  #endif
}
