LiquidCrystalNew
================

Universal library for alphanumeric lcd displays

This library it's basically LiquidCrystal440 http://code.google.com/p/liquidcrystal440/
Modified in many parts and I added the ability to drive LCD's with an external GPIO
(MCP23s16 for SPI or MCP23017 for I2C) that allow to drive even 2 chip big LCD displays
with 2 or 3 wires. Apart that, the main differences with the classic LiquidCrystal is
the ability to drive big displays with 2 or more chips, a correct cursor handle and better
overall timing so it should be faster.
