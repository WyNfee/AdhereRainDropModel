@echo off

CALL buildATGFontHLSL.bat %1 %2 %3 %4 %5 %6 %7 %8
CALL buildDrawHLSL.bat %1 %2 %3 %4 %5 %6 %7 %8
CALL buildMeshHLSL.bat %1 %2 %3 %4 %5 %6 %7 %8
CALL buildVisualizeKinectHLSL.bat %1 %2 %3 %4 %5 %6 %7 %8
CALL buildGenerateMips.bat %1 %2 %3 %4 %5 %6 %7 %8