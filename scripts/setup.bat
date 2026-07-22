@echo off
REM ============================================
REM  RM Team Embedded Dev Environment - One-Click Setup Launcher
REM  Just double-click this file. It will request administrator
REM  privileges and run setup.ps1 automatically.
REM ============================================
cd /d "%~dp0"
echo Starting setup script with administrator privileges...
powershell -NoProfile -Command "Start-Process powershell -Verb RunAs -ArgumentList '-NoExit','-ExecutionPolicy','Bypass','-File','\"%~dp0setup.ps1\"'"
