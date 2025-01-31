@echo off
SET MODEL_NAME=%~n1
XMB_GMO %MODEL_NAME%.tga %MODEL_NAME%.ply %MODEL_NAME%.txt mdl_bg.gmo
pause 