# JRestore
JRestore is a Windows application written in C++. It uses the Windows API and it does not use any external dependencies.

![Screenshot](https://i.imgur.com/2N9vHDi.png)

The purpose of this application is to save the position and size of every window using `GetWindowPlacement()`. Upon clicking the `Restore` button, the window layout is restored.

This is useful in situations such as redocking a laptop to external monitor or temporarily changing the screen resolution. When undocking, Windows automatically moves all applications to the laptop monitor, and when redocking, the applications stay on the laptop monitor.