#!/bin/bash

swig -csharp -c++ -w314,315,401,451,503,516 -I../../../submodules/fmt/include -outfile DotNetTypes/dotnet_quests.cs questinterface.i