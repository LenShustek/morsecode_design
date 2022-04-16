For a house we own in San Francisco I designed a custom security gate 
whose hole pattern contains a "secret" message encoded in Morse Code. 
(Think Jeanette MacDonald.) 

This repository contains the program that designed the hole pattern. It 
accepts a multi-panel description of areas to contain the message. The 
holes can be either left- or right-justified in each panel. You can also 
specify restricted areas where no message is to be placed, which I used 
to leave room for the lockset, the intercom, and the jambs between the 
doors. 

The program is written in the C subset of C++ and was compiled using 
Visual Studio on a Windows PC. The output is a script file that can be 
read into AUTOCAD. Our metal fabricator was able to use that to control 
his laser cutter. 

Len Shustek
January 2021
