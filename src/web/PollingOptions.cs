namespace web
{
    public class PollingOptions
    {
        public string ServiceUuid { get; set; } = "0000181a-0000-1000-8000-00805f9b34fb";
        public string DeviceAddress { get; set; } = "B5:01:34:D0:95:4C";
        public string DeviceName { get; set; } = "Bonsai Sense";
        public string PressureCharacteristicUuid { get; set; } = "00002a6d-0000-1000-8000-00805f9b34fb";
        public string TemperatureCharacteristicUuid { get; set; } = "00002a6e-0000-1000-8000-00805f9b34fb";
        public string HumidityCharacteristicUuid { get; set; } = "00002a6f-0000-1000-8000-00805f9b34fb";
        public double ReconnectIntervalSeconds { get; set; } = 15;
    }
}