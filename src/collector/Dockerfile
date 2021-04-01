FROM mcr.microsoft.com/dotnet/runtime-deps:5.0-buster-slim-arm32v7 AS base

RUN apt-get update
RUN apt-get install -y bluez bluetooth

FROM mcr.microsoft.com/dotnet/sdk:5.0 AS build
WORKDIR /src
COPY . .
RUN dotnet publish \
    --runtime linux-arm --configuration Release \
    --output /app --self-contained true /p:PublishTrimmed=true
COPY docker_entrypoint.sh /app

FROM base AS final
WORKDIR /app
COPY --from=build /app .

ENTRYPOINT sh docker_entrypoint.sh