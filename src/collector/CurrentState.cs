using System;

namespace BonsaiSense.Collector
{
    public class CurrentState
    {
        public double? Temperature { get; set; }
        public double? Pressure { get; set; }
        public double? Humidity { get; set; }
        public ushort? LedsHue { get; set; }
        public byte? LedsSaturation { get; set; }
        public byte? LedsValue { get; set; }

        public void UpdateLeds(ushort? hue, byte? saturation, byte? value)
        {
            var handler = LedsUpdated;
            handler?.Invoke(this, new LedsUpdatedEventArgs
            {
                Hue = hue,
                Saturation = saturation,
                Value = value
            });

            LedsHue = hue;
            LedsSaturation = saturation;
            LedsValue = value;
        }

        public event EventHandler<LedsUpdatedEventArgs> LedsUpdated;
    }
}