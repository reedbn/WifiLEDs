//353 characters would be 3 digit values for every color value,
//four digits for every numeric value, which is unlikely
char wifiBuff[353];

/******************************************************************
*******************************************************************
  Main functions
*******************************************************************
******************************************************************/

void wiflySetup()
{
  /*
  //Set up WiFly
  debugSerial.println(F("Setting up Wifly"));
  
  //Enter command mode
  Serial.print(F("$$$"));delay(10);
  if(!Serial.find(PSTR("CMD")))
  {
    debugSerial.println(F("Baudrate not right"));
  }
  
  //Set the SSID
  Serial.print(F("set wlan ssid "));
  Serial.print(F("PleaseInsertCoin\r"));delay(10);
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't set SSID"));
  }
  
  //Set the passphrase
  Serial.print(F("set wlan phrase "));
  Serial.print(F("player1start\r"));delay(10);
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't set password"));
  }
  
  //Set WPA mode
  /*0 Open (Default)
    1 WEP-128
    2 WPA1
    3 Mixed WPA1 and WPA2-PSK
    4 WPA2-PSK
    5 Not used
    6 Ad hoc mode (join any ad hoc network)
    8 WPE-64*/
  /*Serial.print(F("set wlan auth 4\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't set WPA"));
  }
  
  //Set channel to "any"
  Serial.print(F("set wlan channel 0\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't set channel"));
  }
  
  //Set to join the AP automatically
  Serial.print(F("set wlan join 1\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't join"));
  }
  
  //Turn DHCP on
  Serial.print(F("set ip dhcp 1\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't turn on DHCP"));
  }
  
  //Set the listening port to 80 (http)
  Serial.print(F("set ip localport 80\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't set port 80"));
  }
  
  //Set device id
  Serial.print(F("set opt deviceid LED-Strip\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't set name"));
  }
  
  //Turn off UDP broadcasting
  Serial.print(F("set broadcast interval 0\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't turn off broadcast"));
  }
  
  //Clear any connections we may have right now
  Serial.print(F("close\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't close existing connection"));
  }
  
  //Save current configuration
  Serial.print(F("save\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't set save"));
  }
  
  //Exit command mode
  Serial.print(F("exit\r"));
  if(!Serial.find(PSTR(">")))
  {
    debugSerial.println(F("Couldn't exit command mode"));
  }*/
  
  flushRx();
  closeRx();
  
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(F("Wifly Setup Complete"));
  #endif
}

/**********************************
     Wifi Loop Procedure
**********************************/

void wiflyLoop()
{
  Serial.readBytes(wifiBuff,10);//Should be able to get GET or POST in 10 bytes
  boolean pageFound = false;
  for(int i=0;i<sizeof(wifiBuff)-3;i++)//-3 to account for the +3 for POST
  {
    if(wifiBuff[i] == 'G')
    {
      if(wifiBuff[i+1] == 'E')
      {
        if(wifiBuff[i+2] == 'T')
        {
          #if PRINT_DEBUGGING_WIFLY
          debugSerial.println(F("Got GET"));
          #endif
          pageFound = true;
          sendIndex();
          break;
        }
      }
    }
    else if(wifiBuff[i] == 'P')
    {
      if(wifiBuff[i+1] == 'O')
      {
        if(wifiBuff[i+2] == 'S')
        {
          if(wifiBuff[i+3] == 'T')
          {
            #if PRINT_DEBUGGING_WIFLY
            debugSerial.println(F("Got POST"));
            #endif
            processPost();
            pageFound = true;
            sendIndex();
            break;
          }
        }
      }
    }
  }
  if(!pageFound)
  {
    #if PRINT_DEBUGGING_WIFLY
    debugSerial.println(F("No GET/POST found. Sending 404"));
    printAll();
    #endif
    send404();
  }
  
  if(!Serial.find(PSTR("OS*"),3))//Try to get a normal close
  {
    closeRx();
  }
  flushRx();
}


/******************************************************************
*******************************************************************
  Utility Functions
*******************************************************************
******************************************************************/

//Goes through and sets settings when POST received
void processPost()
{
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

void flushRx()
{
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(F("flushRx()"));
  #endif
  uint32_t start = millis();
  while(millis() - start < 500)
  {
    Serial.read();
  }
}

void closeRx()
{
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(F("closeRx()"));
  #endif
  Serial.print(F("$$$"));
  Serial.find(PSTR("CMD"),3);
  Serial.print(F("close\r"));
  Serial.find(PSTR("OS*"),3);
  Serial.print(F("exit\r"));
  Serial.find(PSTR("T"));//EXIT/**/
}

#if PRINT_DEBUGGING_WIFLY
void printAll()
{
  uint32_t start = millis();
  while((millis() - start < 500))
  {
    if(Serial.available() > 0)
    {
      debugSerial.print((char)Serial.read());
    }
  }
  debugSerial.println();
}
#endif

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
    Serial.println(strlen_P((const prog_char *)str)+2,HEX);
    Serial.println(str);
    Serial.println();
}
void sendChunkln()
{
    Serial.println('0');
    Serial.println();
}

void sendChunk(const __FlashStringHelper *str)
{
    Serial.println(strlen_P((const prog_char *)str),HEX);
    Serial.println(str);
}
void sendChunk(const char *str)
{
    Serial.println(strlen(str),HEX);
    Serial.println(str);
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
  Serial.println(F("HTTP/1.1 404 Not Found"));
  Serial.println(F("Content-Type: text/html"));
  Serial.println(F("Transfer-Encoding: chunked"));
  Serial.println();
  
  //Actual html
  sendChunkln(F("<html><head>"));
  sendChunkln(F("<title>404 Not Found (I Probably Messed Up, Sorry!)</title>"));
  sendChunkln(F("</head><body>"));
  sendChunkln(F("<h1>Not Found</h1>"));
  sendChunkln(F("<hr>"));
  sendChunkln(F("<a href=\"/\">Try Again</a>"));
  sendChunkln(F("</body></html>"));
  sendChunkln();
}

/** Send the form */
void sendIndex()
{
  //Page header
  Serial.println(F("HTTP/1.1 200 OK"));
  Serial.println(F("Content-Type: text/html"));
  Serial.println(F("Transfer-Encoding: chunked"));
  Serial.println();
  
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
  #if PRINT_DEBUGGING_WIFLY
  debugSerial.println(F("Index Page Sent"));
  #endif
}
