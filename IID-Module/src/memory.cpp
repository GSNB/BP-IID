#include <FS.h>
#include "SPIFFS.h"
#include <string>

bool saveToFile(const char *filename, const char *text)
{
  File f = SPIFFS.open(filename, "w");

  if (!f)
  {
    Serial.println("file open failed");
    return false;
  }
  else
  {
    //Write data to file
    Serial.println("Writing Data to File");
    Serial.println(text);
    f.print(text);
    f.close(); //Close file
    return true;
  }
}

std::string readFromFile(const char *filename)
{
  std::string text = "";
  File f = SPIFFS.open(filename, "r");

  if (!f)
  {
    Serial.println("file open failed");
  }
  else
  {
    Serial.println("Reading Data from File:");
    text = f.readStringUntil('\n').c_str();
    Serial.println(text.c_str());
    f.close();
  }
  return text;
}

bool startSPIFFS() {
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }
}