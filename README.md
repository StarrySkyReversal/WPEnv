# Integrated Software Suite for PHP, MySQL, NGINX, and Apache

This project provides an integrated software solution for setting up PHP, MySQL, NGINX, and Apache on Windows, developed using Visual Studio 2022.

The environment for developing this program is Windows 11, It has not yet been tested for running on Windows 10 or Windows 7 systems.

## About Use
The program comes with Apache httpd 2.4.58 and nginx 1.24.0 pre-installed.
For PHP and MySQL, manual downloading is required due to their large package sizes.
This can be done by clicking a specific download button within the program.
The download URLs for PHP and MySQL are found in the 'config/service_source.txt' file in the program's root directory.
These URLs are customizable and can be modified as needed.

## Extensions
Dependencies(vcpkg)

curl:x64-windows  
jansson:x64-windows  
minizip:x64-windows  
openssl:x64-windows  

## Application UI
![Example Image](application_ui.jpg)

## License
This project is licensed under the MIT License. See the LICENSE.md file for details.
