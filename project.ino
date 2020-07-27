const int outputStop = 13;
const int inputStop =  4;

//power, [0] = output pin, [1] = initial value
const int power[]={
  12,//SolarVictron
  11,//SolarBluesun
  10,//CRE
};
const int powercount = sizeof(power) / sizeof(power[0]);
unsigned long lastDown[powercount];

//the first input need match power[], [0] = input pin, [1] = initial value
const int inputs[][2]={
  {7,false},//SolarVictron
  {6,false},//SolarBluesun
  {5,false},//CRE
  {inputStop,true},//stop
};
const int inputcount = sizeof(inputs) / sizeof(inputs[0]);
const int valueCount=64;
//stop default to true, value: power, stop
int indexLastValues=0;
bool inputValues[inputcount][valueCount];
bool lastValues[inputcount];
int sumValuesTrue[inputcount];

const int lowValueHysteresis=30;
const int highValueHysteresis=70;

int lastPowerNumber = -1;
long previousMillis = 0;        // will store last time was updated
long previousMillisSerial = 0;
long interval = 1000;
long upIfLastDownIsGreaterMsThan = 10000;

void setup() {
  pinMode(inputStop, INPUT);
  pinMode(outputStop, OUTPUT);
  
  for( int i = 0; i < powercount; i++ )
      pinMode(inputs[i][0], INPUT);
  for( int i = 0; i < inputcount; i++ )
  {
    if(!inputs[i][1])
    {
      sumValuesTrue[i]=0;
      lastValues[i]=false;
      for( int j = 0; j < valueCount; j++ )
        inputValues[i][j]=0;
    }
    else
    {
      sumValuesTrue[i]=valueCount;
      lastValues[i]=true;
      for( int j = 0; j < valueCount; j++ )
        inputValues[i][j]=1;
    }
  }
  
  for( int i = 0; i < powercount; i++ )
  {
    lastDown[i]=0;
    // set the digital pin as output:
    pinMode(power[i], OUTPUT);
    digitalWrite(power[i], HIGH);
    //VERY IMPORTANT due to delay of relay to change, if drop then short circuit damage
    //shorter delay to init faster, presume all relay was in position of no energy
    delay(100);
  }
}

void loop()
{
  unsigned long currentMillis = millis();
    
  //parse the input with hysteresis + average to fix some corrupted input
  for( int i = 0; i < inputcount; i++ )
  {
    int v = digitalRead(inputs[i][0]);
    bool t=inputValues[i][indexLastValues];
    if(t==true)
      sumValuesTrue[i]--;
    if(v==HIGH)
      sumValuesTrue[i]++;
    bool l=lastValues[i];
    if(l==false && sumValuesTrue[i]*100/valueCount>highValueHysteresis)
      lastValues[i]=true;
    else if(l==true && sumValuesTrue[i]*100/valueCount<lowValueHysteresis)
    {
      lastValues[i]=false;
      if(i<3)
        lastDown[i]=currentMillis;
      else if(lastDown[i]>currentMillis) //in the future, then time drift, fix by 0 because no action before 10s (see upIfLastDownIsGreaterMsThan)
        lastDown[i]=0;
    }
    inputValues[i][indexLastValues]=v;
  }
  indexLastValues++;
  if(indexLastValues>=valueCount)
    indexLastValues=0;
  
  int stopVar = lastValues[3];
  digitalWrite(outputStop, stopVar);
  if(stopVar==HIGH)
  {
    for( int i = 0; i < powercount; i++ )
      digitalWrite(power[i], HIGH);
    lastPowerNumber = -1;
    previousMillis = currentMillis;
  }
  else
  {
    //detect down
    for( int i = 0; i < powercount; i++ )
    {
      if(lastValues[i]==false)
        lastDown[i]=currentMillis;
    }
    
    if(currentMillis>upIfLastDownIsGreaterMsThan && currentMillis - previousMillis > interval)
    {
      int bestPowerNumber=-1;
      //power is considered as UP only if no down during the last 10s
      for( int i = 0; i < powercount; i++ )
      {
        if(lastDown[i]<(currentMillis-upIfLastDownIsGreaterMsThan))
        {
          bestPowerNumber=i;
          break;
        }
      }
      if(bestPowerNumber!=lastPowerNumber)
      {
        if(lastPowerNumber!=-1)
        {
            for( int i = 0; i < powercount; i++ )
              digitalWrite(power[i], HIGH);
            //VERY IMPORTANT due to delay of relay to change, if drop then short circuit damage
            lastPowerNumber = -1;
            previousMillis = currentMillis;
        }
        else
        {
          if(bestPowerNumber>=0 && bestPowerNumber<powercount)
            digitalWrite(power[bestPowerNumber], LOW);
          lastPowerNumber=bestPowerNumber;
          //prevent change during 1s
          previousMillis = currentMillis;
        }
      }
    }
  }
  delay(1);
}

