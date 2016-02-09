﻿using System;

namespace CETech
{
    internal static class Program
    {
        /// <summary>
        ///     The main entry point for the application.
        /// </summary>
        [STAThread]
        private static void Main()
        {
            if (BigInit())
            {
                Application.Run();
            }

            BigShutdown();
        }

        private static bool BigInit()
        {
            Log.LogEvent += LogHandler.console_log;

            Keyboard.Init();
            Application.Init();
            return true;
        }

        private static void BigShutdown()
        {
            Application.Shutdown();
        }
    }
}