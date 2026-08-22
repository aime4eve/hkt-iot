del *.bak /s
del *.ddk /s
del *.edk /s
del *.lst /s
del *.lnp /s
del *.mpf /s
del *.mpj /s
del *.obj /s
del *.omf /s
::del *.opt /s  ::不允许删除JLINK的设置
del *.plg /s
del *.rpt /s
del *.tmp /s
del *.__i /s
del *.crf /s /F
del *.o /s
del *.d /s
del *.axf /s
del *.tra /s
del *.dep /s

del JLinkLog.txt /s

del *.iex /s
del *.htm /s
del *.sct /s
del *.map /s

move .\Objects\*.hex  .\Firmware\
move .\Objects\*.bin  .\Firmware\
move .\Objects_BootLoad\*.hex  .\Firmware\
:: move .\Objects_BootLoad\*.bin  .\Firmware\

rd  /Q /S .\Objects
rd  /Q /S .\Objects_BootLoad
rd 	/Q /S .\Listings
rd 	/Q /S .\Listings_BootLoad
rd  /Q /S .\DebugConfig

exit
