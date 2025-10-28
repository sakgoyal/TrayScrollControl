# Tray Scroll Control

Show an example for a minimal tray icon that receives scroll events. 
it makes sure to register and unregister on move movements. 
It uses the `Shell_NotifyIcon` and `Shell_NotifyIconGetRect` API from windows.

because the tray icon does not expose scroll events natively, you need to get the icon 
bounding box and manually check when the mouse is scrolling inside that RECT. 


It has a context menu to exit the program as an example. 
