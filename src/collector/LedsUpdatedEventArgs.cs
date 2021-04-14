using System;

namespace BonsaiSense.Collector
{
    public sealed class LedsUpdatedEventArgs : EventArgs
    {
        public ushort? Hue { get; set; }
        public byte? Saturation { get; set; }
        public byte? Value { get; set; }
    }
}