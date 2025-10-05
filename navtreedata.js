/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "LD2410Async", "index.html", [
    [ "Installation", "Installation.html", [
      [ "Introduction", "index.html#autotoc_md37", null ],
      [ "Features", "index.html#autotoc_md38", null ],
      [ "Documentation", "index.html#autotoc_md39", null ],
      [ "Main Class Reference", "index.html#autotoc_md40", null ],
      [ "Examples", "index.html#autotoc_md41", null ],
      [ "Using Arduino Library Manager (recommended)", "Installation.html#Installation_LibManager", null ],
      [ "Manual installation", "Installation.html#Installation_Manual", null ]
    ] ],
    [ "Async Commands & Processing", "Async_Commands_And_Processing.html", [
      [ "Why asynchronous processing matters for the LD2410", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_Why", null ],
      [ "Detection Data Callback", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_Detection_Data_Callback", null ],
      [ "Detection Data Callback Example", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_Dtection_Data_Callback_Example_", null ],
      [ "Configuration Callbacks", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_Configuration_Callbacks", null ],
      [ "Configuration Callbacks Example", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_Configuration_Callbacks_Exampample", null ],
      [ "Async Commands Basics", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_Async_Commands_Basics", null ],
      [ "List of Asynchronous Commands", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_Async_Commands_List", null ],
      [ "Async Command Example", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_AsyncCommands__Example", null ],
      [ "Best Practices for Async Commands", "Async_Commands_And_Processing.html#Async_Commands_And_Processing_Async_Commands_Best_Practices", null ]
    ] ],
    [ "Data Structures", "Data_Structures.html", [
      [ "DetectionData", "Data_Structures.html#Data_Structures_DetectionData", [
        [ "Members", "Data_Structures.html#Data_Structures_DetectionData_Members", [
          [ "General", "Data_Structures.html#Data_Structures_DetectionData_General", null ],
          [ "Basic Detection Results", "Data_Structures.html#Data_Structures_DetectionData_Basic", null ],
          [ "Engineering Mode Data", "Data_Structures.html#Data_Structures_DetectionData_Engineering", null ]
        ] ]
      ] ],
      [ "Data_Structures_ConfigData", "Data_Structures.html#Data_Structures_ConfigData", [
        [ "Members", "Data_Structures.html#Data_Structures_ConfigData_Members", [
          [ "Radar Gates", "Data_Structures.html#Data_Structures_ConfigData_Members_Gates", null ],
          [ "Timeout", "Data_Structures.html#Data_Structures_ConfigData_Members_Timeout", null ],
          [ "Distance Resolution", "Data_Structures.html#Data_Structures_ConfigData_Members_Distance_Resolution", null ],
          [ "Auxiliary Controls", "Data_Structures.html#Data_Structures_ConfigData_Members_Aux", null ]
        ] ]
      ] ]
    ] ],
    [ "Operation Modes", "Operation_Modes.html", [
      [ "Normal Detection Mode", "Operation_Modes.html#Operation_Modes_Normal_Mode", null ],
      [ "Engineering Mode", "Operation_Modes.html#Operation_Modes_Engineering_Mode", null ],
      [ "Configuration Mode", "Operation_Modes.html#Operation_Modes_Configuration_Mode", null ]
    ] ],
    [ "Inactivity Handling", "Inactivity_Handling.html", [
      [ "Basics", "Inactivity_Handling.html#Inactivity_Handling_Basics", null ],
      [ "Enabling / Disabling Inactivity Handling", "Inactivity_Handling.html#Inactivity_Handling_EnableDisable", null ],
      [ "Timeout Configuration", "Inactivity_Handling.html#Inactivity_Handling_Timeout", null ]
    ] ],
    [ "Examples", "Examples.html", [
      [ "Example: Using callback for presence detection updates", "Examples.html#Examples_Callback", null ],
      [ "Example: Clone config data, modify, and write back", "Examples.html#Examples_Modify_Config", null ],
      [ "Usage Example Sketches", "Examples.html#Examples_Usage", null ],
      [ "Test Sketches", "Examples.html#Examples_Test", null ]
    ] ],
    [ "Important Notes and Best Practices", "BestPractices.html", null ],
    [ "Troubleshooting Guide", "Troubleshooting.html", [
      [ "Problem Scenarios", "Troubleshooting.html#Troubleshooting_Problems", [
        [ "No Data Received", "Troubleshooting.html#autotoc_md42", null ],
        [ "Callbacks Not Firing", "Troubleshooting.html#autotoc_md43", null ],
        [ "Async Commands Not Working", "Troubleshooting.html#autotoc_md44", null ],
        [ "Unexpected Sensor Reboots", "Troubleshooting.html#autotoc_md45", null ],
        [ "Data Loss", "Troubleshooting.html#autotoc_md47", null ],
        [ "Strange or Invalid Data", "Troubleshooting.html#autotoc_md49", null ]
      ] ],
      [ "Debugging", "Troubleshooting.html#Troubleshooting_Debugging", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"Async_Commands_And_Processing.html",
"classLD2410Async.html#ab0d3227098f6b8ecb3ed35ce3d114e04",
"structLD2410Types_1_1ConfigData.html#ad4b3263e4da6bcc2ef64d1e727eb6f06"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';