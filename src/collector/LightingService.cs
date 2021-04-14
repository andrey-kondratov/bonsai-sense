using System;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Hosting;

namespace BonsaiSense.Collector
{
    internal sealed class LightingService : BackgroundService
    {
        private readonly CurrentState _currentState;
        private static readonly (ushort h, byte s, byte v) DayValues = (6500, 96, 112);
        private static readonly (ushort h, byte s, byte v) NightValues = (0, 0, 0);

        public LightingService(CurrentState currentState)
        {
            _currentState = currentState;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                await Task.Delay(1000, stoppingToken);

                if (DateTime.Now.Hour == 11 && DateTime.Now.Minute == 00)
                {
                    SetValues(DayValues);
                }
                else if (DateTime.Now.Hour == 23 && DateTime.Now.Minute == 00)
                {
                    SetValues(NightValues);
                }
            }
        }

        private void SetValues((ushort hue, byte saturation, byte value) values)
        {
            _currentState.UpdateLeds(values.hue, values.saturation, values.value);
        }
    }
}