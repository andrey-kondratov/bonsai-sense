namespace BonsaiSense.Collector
{
    public class PollingOptions
    {
        public string ServiceUuid { get; set; } = "0000181a-0000-1000-8000-00805f9b34fb";
        public string DeviceAddress { get; set; } = "B5:01:34:D0:95:C4";
        public string DeviceName { get; set; } = "Bonsai Sense";
        public string PressureCharacteristicUuid { get; set; } = "00002a6d-0000-1000-8000-00805f9b34fb";
        public string TemperatureCharacteristicUuid { get; set; } = "00002a6e-0000-1000-8000-00805f9b34fb";
        public string HumidityCharacteristicUuid { get; set; } = "00002a6f-0000-1000-8000-00805f9b34fb";
        public string LedsServiceUuid { get; set; } = "BC1DF15A-C3CF-41BB-AF3F-CE7A15949B79";
        public string LedsHueCharacteristicUuid { get; set; } = "F76048A9-6446-49D7-A68D-3A0079D4EADC";
        public string LedsSaturationCharacteristicUuid { get; set; } = "6F8250E5-E137-4365-8266-C9F81D39453B";
        public string LedsValueCharacteristicUuid { get; set; } = "4AD5A9F6-472B-4879-9A8E-789382CD9AFF";
        public double ReconnectIntervalSeconds { get; set; } = 15;
    }
}