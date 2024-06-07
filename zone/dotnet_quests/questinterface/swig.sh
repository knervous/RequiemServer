#!/bin/bash

./swig/swig -v -csharp -c++ -w314,315,401,451,503,516 -I./swig/csharp -I./swig -I../../../submodules/fmt/include -outfile DotNetTypes/dotnet_quests.cs questinterface.i

echo Finished