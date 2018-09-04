set ProjFile=water.Uv2
set TargetName=STC89C52RD

set UVPath=C:\Keil\uv3\UV3
set Path=%UVPath%;%Path%
del /Q	*.obj *.lst *.hex *.plg *.lnp *.m51 *.opt
c:\Keil\uv3\UV3\Uv3 -r %ProjFile% -t "%TargetName%" -o build.log
