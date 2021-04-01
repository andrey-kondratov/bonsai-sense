using System;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using HashtagChris.DotNetBlueZ;
using HashtagChris.DotNetBlueZ.Extensions;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;

namespace BonsaiSense.Collector
{
    internal class PollingService : IHostedService
    {
        private readonly ILogger<PollingService> _logger;
        private readonly CurrentState _currentState;
        private readonly PollingOptions _options;
        private Adapter _adapter;
        private Device _device;
        private GattCharacteristic _pressureCharacteristic;
        private GattCharacteristic _temperatureCharacteristic;
        private GattCharacteristic _humidityCharacteristic;

        public PollingService(ILogger<PollingService> logger,
            CurrentState currentState, IOptions<PollingOptions> options)
        {
            _logger = logger;
            _currentState = currentState;
            _options = options.Value;
        }

        public async Task StartAsync(CancellationToken cancellationToken)
        {
            _logger.LogInformation("Starting the polling service...");

            _logger.LogInformation("Initializing Bluetooth adapter...");
            var adapters = await BlueZManager.GetAdaptersAsync();
            if (!adapters.Any())
            {
                _logger.LogError("No Bluetooth adapters found.");
                return;
            }

            _adapter = adapters.First();
            var adapterPath = _adapter.ObjectPath.ToString();
            var adapterName = adapterPath.Substring(adapterPath.LastIndexOf("/") + 1);
            _logger.LogInformation("Using Bluetooth adapter {AdapterName}",
                adapterName);

            _adapter.PoweredOn += OnAdapterPoweredOnAsync;
            _adapter.DeviceFound += OnAdapterDeviceFoundAsync;

            _logger.LogInformation("Polling service started.");
        }

        private async Task OnAdapterPoweredOnAsync(Adapter sender, BlueZEventArgs eventArgs)
        {
            try
            {
                if (eventArgs.IsStateChange)
                {
                    _logger.LogInformation("Bluetooth adapter powered on.");
                }
                else
                {
                    _logger.LogInformation("Bluetooth adapter already powered on.");
                }

                _logger.LogInformation("Starting discovery...");
                await sender.StartDiscoveryAsync();
            }
            catch (Exception exception)
            {
                _logger.LogError(exception, "Exception on starting discovery.");
            }
        }

        private async Task OnAdapterDeviceFoundAsync(Adapter sender, DeviceFoundEventArgs eventArgs)
        {
            try
            {
                if (_device != null)
                {
                    _logger.LogInformation("Device already initialized. Skipping {@EventArgs}.",
                        eventArgs);
                    return;
                }

                Device1Properties deviceProperties = await eventArgs.Device.GetAllAsync();
                _logger.LogInformation((eventArgs.IsStateChange ? "Found [NEW]: " : "Found: ") +
                    "{Address} (Alias: {Alias}, RSSI: {Rssi})",
                    deviceProperties.Address, deviceProperties.Alias,
                    deviceProperties.RSSI);

                string deviceAddress = await eventArgs.Device.GetAddressAsync();
                string deviceName = await eventArgs.Device.GetAliasAsync();
                if (!string.Equals(deviceAddress, _options.DeviceAddress,
                    StringComparison.OrdinalIgnoreCase) &&
                    !string.Equals(deviceName, _options.DeviceName,
                    StringComparison.OrdinalIgnoreCase))
                {
                    _logger.LogInformation("Device does not match the name or address needed. Skipping.");
                    return;
                }

                _device = eventArgs.Device;

                _logger.LogInformation("Stopping discovery...");
                try
                {
                    await sender.StopDiscoveryAsync();
                    _logger.LogInformation("Discovery stopped.");
                }
                catch (Exception exception)
                {
                    _logger.LogError(exception, "Failed to stop the discovery.");
                }

                _device.Connected += OnDeviceConnectedAsync;
                _device.Disconnected += OnDeviceDisconnectedAsync;
                _device.ServicesResolved += OnDeviceServicesResolvedAsync;

                _logger.LogInformation("Connecting to {Address}", deviceAddress);
                await _device.ConnectAsync();
            }
            catch (Exception exception)
            {
                _logger.LogError(exception, "Error handling the DeviceFound event.");

                _device = null;
                eventArgs.Device.Connected -= OnDeviceConnectedAsync;
                eventArgs.Device.Disconnected -= OnDeviceDisconnectedAsync;
                eventArgs.Device.ServicesResolved -= OnDeviceServicesResolvedAsync;
            }
        }

        private async Task OnDeviceConnectedAsync(Device sender, BlueZEventArgs eventArgs)
        {
            try
            {
                if (eventArgs.IsStateChange)
                {
                    _logger.LogInformation("Connected to {Address}.",
                        await sender.GetAddressAsync());
                }
                else
                {
                    _logger.LogInformation("Already connected to {Address}.",
                        await sender.GetAddressAsync());
                }
            }
            catch (Exception exception)
            {
                _logger.LogError(exception, "Error handling the DeviceConnected event.");
            }
        }

        private async Task OnDeviceDisconnectedAsync(Device sender, BlueZEventArgs eventArgs)
        {
            try
            {
                string address = await sender.GetAddressAsync();
                _logger.LogInformation("Disconnected from {Address}.", address);

                await Task.Delay(TimeSpan.FromSeconds(_options.ReconnectIntervalSeconds));

                _logger.LogInformation("Attempting to reconnect to {Address}...",
                    address);

                await sender.ConnectAsync();
            }
            catch (Exception exception)
            {
                _logger.LogError(exception, "Error handling DeviceDisconnected event");
            }
        }

        private async Task OnDeviceServicesResolvedAsync(Device sender, BlueZEventArgs eventArgs)
        {
            try
            {
                string address = await sender.GetAddressAsync();
                if (eventArgs.IsStateChange)
                {
                    _logger.LogInformation("Services resolved for {Address}.",
                        address);
                }
                else
                {
                    _logger.LogInformation("Services already resolved for {Address}.",
                        address);
                }

                IGattService1 service = await sender.GetServiceAsync(_options.ServiceUuid);
                if (service == null)
                {
                    _logger.LogError("Service UUID {Uuid} not found. Do you need to pair first?",
                        _options.ServiceUuid);
                    await sender.DisconnectAsync();
                    return;
                }

                _pressureCharacteristic = await service
                    .GetCharacteristicAsync(_options.PressureCharacteristicUuid);
                if (_pressureCharacteristic == null)
                {
                    _logger.LogWarning("No pressure characteristic found within service {ServiceUuid}.",
                        _options.ServiceUuid);
                }
                else
                {
                    _pressureCharacteristic.Value += OnPressureValueAsync;
                }

                _temperatureCharacteristic = await service
                    .GetCharacteristicAsync(_options.TemperatureCharacteristicUuid);
                if (_temperatureCharacteristic == null)
                {
                    _logger.LogWarning("No temperature characteristic found with service {ServiceUuid}.",
                        _options.ServiceUuid);
                }
                else
                {
                    _temperatureCharacteristic.Value += OnTemperatureValueAsync;
                }

                _humidityCharacteristic = await service
                    .GetCharacteristicAsync(_options.HumidityCharacteristicUuid);
                if (_humidityCharacteristic == null)
                {
                    _logger.LogWarning("No humidity characteristic found within service {ServiceUuid}.",
                        _options.ServiceUuid);
                }
                else
                {
                    _humidityCharacteristic.Value += OnHumidityValueAsync;
                }
            }
            catch (Exception exception)
            {
                _logger.LogError(exception, "Error handling ServicesResolved event.");
            }
        }

        private Task OnPressureValueAsync(GattCharacteristic sender, GattCharacteristicValueEventArgs eventArgs)
        {
            double value = Math.Round(BitConverter.ToUInt32(eventArgs.Value) * .1, 4);
            _logger.LogDebug("Received raw pressure value: {@ValueBytes}, parsed: {Value} Pa.",
                eventArgs.Value, value);

            _currentState.Pressure = value;

            return Task.CompletedTask;
        }

        private Task OnTemperatureValueAsync(GattCharacteristic sender, GattCharacteristicValueEventArgs eventArgs)
        {
            double value = Math.Round(BitConverter.ToInt16(eventArgs.Value) * .01, 4);
            _logger.LogDebug("Received raw temperature value: {@ValueBytes}, parsed: {Value} °C.",
                eventArgs.Value, value);

            _currentState.Temperature = value;

            return Task.CompletedTask;
        }

        private Task OnHumidityValueAsync(GattCharacteristic sender, GattCharacteristicValueEventArgs eventArgs)
        {
            double value = Math.Round(BitConverter.ToUInt16(eventArgs.Value) * .01, 4);
            _logger.LogDebug("Received raw humidity value: {@ValueBytes}, parsed: {Value} %.",
                eventArgs.Value, value);

            _currentState.Humidity = value;

            return Task.CompletedTask;
        }

        public async Task StopAsync(CancellationToken cancellationToken)
        {
            _logger.LogInformation("Stopping the polling service...");

            if (_humidityCharacteristic != null)
            {
                _humidityCharacteristic.Value -= OnHumidityValueAsync;
            }

            if (_pressureCharacteristic != null)
            {
                _pressureCharacteristic.Value -= OnPressureValueAsync;
            }

            if (_temperatureCharacteristic != null)
            {
                _temperatureCharacteristic.Value -= OnTemperatureValueAsync;
            }

            if (_device != null)
            {
                _device.ServicesResolved -= OnDeviceServicesResolvedAsync;
                _device.Disconnected -= OnDeviceDisconnectedAsync;
                _device.Connected -= OnDeviceConnectedAsync;

                await _device.DisconnectAsync();
            }

            if (_adapter != null)
            {
                _adapter.DeviceFound -= OnAdapterDeviceFoundAsync;
                _adapter.PoweredOn -= OnAdapterPoweredOnAsync;

                await _adapter.StopDiscoveryAsync();
            }

            _logger.LogInformation("Polling service stopped.");
        }
    }
}