#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

constexpr uint8_t LCD_COLUMNS = 16;
constexpr uint8_t LCD_ROWS = 2;

LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);

String fitDisplayLine(const String &text)
{
  String line = text.substring(0, LCD_COLUMNS);
  while (line.length() < LCD_COLUMNS)
    line += ' ';
  return line;
}

String centerText(const String &text)
{
  String clipped = text.substring(0, LCD_COLUMNS);
  uint8_t leftPadding = (LCD_COLUMNS - clipped.length()) / 2;
  String result;
  for (uint8_t i = 0; i < leftPadding; i++)
    result += ' ';
  result += clipped;
  return result;
}

void lcdSetup()
{
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void setDisplayLines(const String &line1, const String &line2)
{
  lcd.setCursor(0, 0);
  lcd.print(fitDisplayLine(line1));
  lcd.setCursor(0, 1);
  lcd.print(fitDisplayLine(line2));
}
