#include "ConfigurationWebServer.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Update.h>

static constexpr const char* GitHubOwner = "jabelpc";
static constexpr const char* GitHubRepo = "flyradar";
static constexpr const char* GitHubBinAssetName = "flyradar.bin";
static constexpr const char* GitHubUserAgent = "FlyRadarOTA/1.0";
static constexpr const char* CurrentFirmwareVersion = "1.0.0";

struct GitHubReleaseInfo {
    bool success;
    String latestVersion;
    String downloadUrl;
    String errorMessage;
};

static String NormalizeVersion(String version)
{
    if (version.startsWith("v") || version.startsWith("V")) {
        return version.substring(1);
    }
    return version;
}

static int CompareVersions(const String& a, const String& b)
{
    const String normalizedA = NormalizeVersion(a);
    const String normalizedB = NormalizeVersion(b);

    int indexA = 0;
    int indexB = 0;
    while (indexA < normalizedA.length() || indexB < normalizedB.length()) {
        int nextDotA = normalizedA.indexOf('.', indexA);
        int nextDotB = normalizedB.indexOf('.', indexB);

        if (nextDotA == -1) nextDotA = normalizedA.length();
        if (nextDotB == -1) nextDotB = normalizedB.length();

        const String partA = normalizedA.substring(indexA, nextDotA);
        const String partB = normalizedB.substring(indexB, nextDotB);

        const int valueA = partA.toInt();
        const int valueB = partB.toInt();

        if (valueA < valueB) return -1;
        if (valueA > valueB) return 1;

        indexA = nextDotA == normalizedA.length() ? normalizedA.length() : nextDotA + 1;
        indexB = nextDotB == normalizedB.length() ? normalizedB.length() : nextDotB + 1;
    }

    return 0;
}

static GitHubReleaseInfo FetchLatestGithubRelease()
{
    // If the repo owner/repo are left as placeholders, return a clear error
    if (String(GitHubOwner) == "your-github-owner" || String(GitHubRepo) == "your-github-repo") {
        return {false, "", "", "GitHub owner/repo not configured. Set GitHubOwner and GitHubRepo in ConfigurationWebServer.cpp"};
    }
    HTTPClient http;
    const String apiUrl = String("https://api.github.com/repos/") + GitHubOwner + "/" + GitHubRepo + "/releases/latest";
    http.begin(apiUrl);
    http.addHeader("User-Agent", GitHubUserAgent);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    const int responseCode = http.GET();
    if (responseCode != HTTP_CODE_OK) {
        const String errorMessage = String("GitHub API error: ") + responseCode;
        http.end();
        return {false, "", "", errorMessage};
    }

    const String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(8192);
    const DeserializationError parseError = deserializeJson(doc, payload);
    if (parseError) {
        return {false, "", "", String("JSON parse error: ") + parseError.c_str()};
    }

    const String latestVersion = doc["tag_name"].as<String>();
    if (latestVersion.isEmpty()) {
        return {false, "", "", "GitHub release tag not found"};
    }

    const JsonArray assets = doc["assets"].as<JsonArray>();
    if (assets.isNull()) {
        return {false, latestVersion, "", "No release assets found"};
    }

    for (const JsonObject asset : assets) {
        const String name = asset["name"].as<String>();
        if (name == GitHubBinAssetName) {
            const String downloadUrl = asset["browser_download_url"].as<String>();
            return {true, latestVersion, downloadUrl, ""};
        }
    }

    return {false, latestVersion, "", String("No asset named '") + GitHubBinAssetName + "' found"};
}

static bool PerformGithubOTA(const String& downloadUrl, String& errorMessage)
{
    HTTPClient http;
    http.begin(downloadUrl);
    http.addHeader("User-Agent", GitHubUserAgent);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    const int responseCode = http.GET();
    if (responseCode != HTTP_CODE_OK) {
        errorMessage = String("Firmware download error: HTTP ") + responseCode;
        http.end();
        return false;
    }

    const int contentLength = http.getSize();
    if (contentLength <= 0) {
        errorMessage = "Firmware download has invalid content length";
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    if (!Update.begin(contentLength)) {
        errorMessage = String("Update begin failed: ") + Update.errorString();
        http.end();
        return false;
    }

    size_t written = Update.writeStream(*stream);
    if (written != static_cast<size_t>(contentLength)) {
        errorMessage = String("Update write failed: written ") + written + " of " + contentLength;
        Update.end();
        http.end();
        return false;
    }

    if (!Update.end(true)) {
        errorMessage = String("Update end failed: ") + Update.errorString();
        http.end();
        return false;
    }

    http.end();
    return true;
}

// HTML stored in flash
// %PLACEHOLDER% tokens are substituted at serve time by the template processor
static const char CONFIG_HTML[] PROGMEM = R"(
<html>
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Configure FlyRadar</title>
        <script src="https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4.3.0"></script>
    </head>
    <body class="font-mono bg-gray-900 text-green-500 min-h-screen p-4 sm:p-0 text-md sm:text-sm">
        <fieldset class="border border-green-500 p-5 w-full max-w-2xl mx-auto sm:m-10">
            <legend class="px-2">Configure FlyRadar</legend>

            <form id="cfg" action="/save" method="POST" class="flex flex-col gap-4 sm:gap-2">

                <div class="flex flex-col sm:flex-row gap-4 sm:gap-5">
                    <label class="flex flex-col sm:flex-row gap-2 flex-1">
                        <span>Latitude:</span>
                        <input
                            name="latitude"
                            type="number"
                            min="-90"
                            step="0.000001"
                            max="90"
                            value='%LATITUDE%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>

                    <label class="flex flex-col sm:flex-row gap-2 flex-1">
                        <span>Longitude:</span>
                        <input
                            name="longitude"
                            type="number"
                            min="-180"
                            step="0.000001"
                            max="180"
                            value='%LONGITUDE%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>
                </div>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>Radius (in &deg;):</span>
                    <input
                        name="radius"
                        type="number"
                        min="0.000001"
                        step="0.000001"
                        max="2.499999"
                        value='%RADIUS%'
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <div class="grid grid-cols-1 sm:grid-cols-3 gap-4">
                    <label class="flex flex-col gap-2">
                        <span>Circle 1 radius (km):</span>
                        <input
                            name="circle1"
                            type="number"
                            min="0"
                            step="0.1"
                            value='%CIRCLE1%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>
                    <label class="flex flex-col gap-2">
                        <span>Circle 2 radius (km):</span>
                        <input
                            name="circle2"
                            type="number"
                            min="0"
                            step="0.1"
                            value='%CIRCLE2%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>
                    <label class="flex flex-col gap-2">
                        <span>Circle 3 radius (km):</span>
                        <input
                            name="circle3"
                            type="number"
                            min="0"
                            step="0.1"
                            value='%CIRCLE3%'
                            class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                    </label>
                </div>

                <div class="grid grid-cols-1 gap-4">
                    <div class="font-semibold">Points of Interest</div>
                    <div class="grid grid-cols-1 sm:grid-cols-3 gap-4">
                        <div class="border border-green-500 p-3 rounded">
                            <div class="font-semibold mb-2">POI 1</div>
                            <label class="flex flex-col gap-2">
                                <span>Latitude:</span>
                                <input
                                    name="poi1-latitude"
                                    type="number"
                                    min="-90"
                                    max="90"
                                    step="0.000001"
                                    value='%POI1_LATITUDE%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-col gap-2">
                                <span>Longitude:</span>
                                <input
                                    name="poi1-longitude"
                                    type="number"
                                    min="-180"
                                    max="180"
                                    step="0.000001"
                                    value='%POI1_LONGITUDE%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-col gap-2">
                                <span>Name:</span>
                                <input
                                    name="poi1-name"
                                    value='%POI1_NAME%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-row items-center gap-2">
                                <input
                                    name="poi1-enabled"
                                    type="checkbox"
                                    %POI1_ENABLED%
                                    class="accent-pink-500">
                                <span>Afficher</span>
                            </label>
                        </div>
                        <div class="border border-green-500 p-3 rounded">
                            <div class="font-semibold mb-2">POI 2</div>
                            <label class="flex flex-col gap-2">
                                <span>Latitude:</span>
                                <input
                                    name="poi2-latitude"
                                    type="number"
                                    min="-90"
                                    max="90"
                                    step="0.000001"
                                    value='%POI2_LATITUDE%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-col gap-2">
                                <span>Longitude:</span>
                                <input
                                    name="poi2-longitude"
                                    type="number"
                                    min="-180"
                                    max="180"
                                    step="0.000001"
                                    value='%POI2_LONGITUDE%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-col gap-2">
                                <span>Name:</span>
                                <input
                                    name="poi2-name"
                                    value='%POI2_NAME%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-row items-center gap-2">
                                <input
                                    name="poi2-enabled"
                                    type="checkbox"
                                    %POI2_ENABLED%
                                    class="accent-pink-500">
                                <span>Afficher</span>
                            </label>
                        </div>
                        <div class="border border-green-500 p-3 rounded">
                            <div class="font-semibold mb-2">POI 3</div>
                            <label class="flex flex-col gap-2">
                                <span>Latitude:</span>
                                <input
                                    name="poi3-latitude"
                                    type="number"
                                    min="-90"
                                    max="90"
                                    step="0.000001"
                                    value='%POI3_LATITUDE%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-col gap-2">
                                <span>Longitude:</span>
                                <input
                                    name="poi3-longitude"
                                    type="number"
                                    min="-180"
                                    max="180"
                                    step="0.000001"
                                    value='%POI3_LONGITUDE%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-col gap-2">
                                <span>Name:</span>
                                <input
                                    name="poi3-name"
                                    value='%POI3_NAME%'
                                    class="border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                            </label>
                            <label class="flex flex-row items-center gap-2">
                                <input
                                    name="poi3-enabled"
                                    type="checkbox"
                                    %POI3_ENABLED%
                                    class="accent-pink-500">
                                <span>Afficher</span>
                            </label>
                        </div>
                    </div>
                </div>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>OpenSkyAPI Client ID:</span>
                    <input
                        name="opensky-id"
                        value='%OPENSKY_ID%'
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>OpenSkyAPI Client Secret:</span>
                    <input
                        name="opensky-secret"
                        value='%OPENSKY_SECRET%'
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                </label>

                <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                    <span>Color theme:</span>
                    <select
                        name="theme"
                        class="flex-1 border border-green-500 bg-gray-900 w-full px-3 py-2 text-lg sm:text-base sm:px-1 sm:py-0">
                        <option value="green" %THEME_GREEN%>Classic green</option>
                        <option value="cyan" %THEME_CYAN%>Cyan HUD</option>
                        <option value="amber" %THEME_AMBER%>Amber retro</option>
                        <option value="altitude" %THEME_ALTITUDE%>Altitude gradient</option>
                    </select>
                </label>

                <div class="flex flex-col sm:flex-row gap-4 sm:justify-between">
                    <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                        <span>Radar sweep:</span>
                        <input
                            name="scanline"
                            type="checkbox"
                            %SCANLINE%
                            class="px-3 sm:px-1 accent-green-500">
                    </label>
                    <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                        <span>Aircraft Info:</span>
                        <input
                            name="infotext"
                            type="checkbox"
                            %INFOTEXT%
                            class="px-3 sm:px-1 accent-green-500">
                    </label>
                    <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                        <span>Directional Aircraft:</span>
                        <input
                            name="triangle"
                            type="checkbox"
                            %TRIANGLE%
                            class="px-3 sm:px-1 accent-green-500">
                    </label>
                    <label class="flex flex-col sm:flex-row items-start sm:items-center gap-2">
                        <span>North Arrow:</span>
                        <input
                            name="north"
                            type="checkbox"
                            %NORTH%
                            class="px-3 sm:px-1 accent-green-500">
                    </label>
                </div>

                <div class="flex flex-col sm:flex-row gap-4 sm:gap-5">
                    <input
                        type="submit"
                        value="Save"
                        class="bg-green-500 text-black mt-4 px-4 py-3 text-lg sm:text-base sm:px-2 sm:py-0 self-start cursor-pointer">

                    <div id="result" class="mt-4 px-1 sm:px-10"></div>
                </div>
            </form>
        </fieldset>

        <fieldset class="border border-green-500 p-5 w-full max-w-2xl mx-auto sm:m-10 mt-4">
            <legend class="px-2">Firmware Update</legend>
            <div class="flex flex-col gap-3">
                <div id="updateStatus">Vérification de mise à jour...</div>
                <button id="updateButton" type="button" class="bg-green-500 text-black px-4 py-3 text-lg sm:text-base sm:px-2 sm:py-0 self-start cursor-pointer hidden">
                    Mettre à jour vers <span id="updateVersion"></span>
                </button>
                <div id="updateResult" class="mt-2"></div>
            </div>
        </fieldset>

        <script>
            document.getElementById('cfg').addEventListener('submit', function(e) {
                e.preventDefault();
                fetch(this.action, { method: 'POST', body: new FormData(this) })
                    .then(r => r.text())
                    .then(html => document.getElementById('result').innerHTML = html);
            });

            async function refreshUpdateStatus() {
                const status = document.getElementById('updateStatus');
                const button = document.getElementById('updateButton');
                const version = document.getElementById('updateVersion');
                const result = document.getElementById('updateResult');
                result.textContent = '';

                try {
                    const response = await fetch('/check-update');
                    if (!response.ok) {
                        throw new Error(await response.text());
                    }
                    const data = await response.json();

                    if (data.updateAvailable) {
                        status.textContent = `Nouvelle version disponible : ${data.latestVersion} (actuelle ${data.currentVersion})`;
                        version.textContent = data.latestVersion;
                        button.style.display = 'inline-flex';
                        button.disabled = false;
                    } else {
                        status.textContent = `Aucune mise à jour disponible. Version actuelle : ${data.currentVersion}`;
                        button.style.display = 'none';
                    }
                } catch (error) {
                    status.textContent = `Erreur de vérification : ${error}`;
                    button.style.display = 'none';
                }
            }

            document.getElementById('updateButton').addEventListener('click', async function() {
                const button = document.getElementById('updateButton');
                const result = document.getElementById('updateResult');
                button.disabled = true;
                result.textContent = 'Application de la mise à jour...';

                try {
                    const response = await fetch('/apply-update', { method: 'POST' });
                    const text = await response.text();
                    if (!response.ok) {
                        throw new Error(text);
                    }
                    result.textContent = text;
                } catch (error) {
                    result.textContent = `Erreur : ${error}`;
                }
            });

            refreshUpdateStatus();
        </script>
    </body>
</html>
)";

void ConfigurationWebServer::Initialise() {
    // start mDNS and check result
    if (!MDNS.begin("flyradar")) {
        Serial.println("[WARN] Failed to start mDNS. Continuing without mDNS...");
    }

    // Handle visit to config web server
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
        Serial.println("[GET] Handling request to config web server...");

        // read all values up front so the processor lambda can capture by value
        prefs.begin("config", true);
        const String latitude = prefs.getString("latitude", "");
        const String longitude = prefs.getString("longitude", "");
        const String radius = prefs.getString("radius", "1.0");
        const String openskyClientId = prefs.getString("opensky-id", "");
        String openskySecret = prefs.getString("opensky-secret", "");
        const String scanlineEnabled = prefs.getString("scanline", "true");
        const String infoTextEnabled = prefs.getString("infotext", "true");
        const String triangleEnabled = prefs.getString("triangle", "true");
        const String northEnabled = prefs.getString("north", "true");
        const String poi1Latitude = prefs.getString("poi1-latitude", "");
        const String poi1Longitude = prefs.getString("poi1-longitude", "");
        const String poi1Name = prefs.getString("poi1-name", "");
        const String poi1Enabled = prefs.getString("poi1-enabled", "true");
        const String poi2Latitude = prefs.getString("poi2-latitude", "");
        const String poi2Longitude = prefs.getString("poi2-longitude", "");
        const String poi2Name = prefs.getString("poi2-name", "");
        const String poi2Enabled = prefs.getString("poi2-enabled", "true");
        const String poi3Latitude = prefs.getString("poi3-latitude", "");
        const String poi3Longitude = prefs.getString("poi3-longitude", "");
        const String poi3Name = prefs.getString("poi3-name", "");
        const String poi3Enabled = prefs.getString("poi3-enabled", "true");
        const String theme = prefs.getString("theme", "green");
        const String circle1 = prefs.getString("circle1", "10");
        const String circle2 = prefs.getString("circle2", "30");
        const String circle3 = prefs.getString("circle3", "50");
        prefs.end();

        // mask secret before sending to client
        std::fill(openskySecret.begin(), openskySecret.end(), '*');

        // template processor called once per %PLACEHOLDER% token found in CONFIG_HTML.
        AsyncWebServerResponse* response = request->beginResponse(
            200, "text/html",
            (const uint8_t*)CONFIG_HTML, sizeof(CONFIG_HTML) - 1,
            [latitude, longitude, radius, openskyClientId, openskySecret, scanlineEnabled, infoTextEnabled, triangleEnabled, northEnabled, poi1Latitude, poi1Longitude, poi1Name, poi1Enabled, poi2Latitude, poi2Longitude, poi2Name, poi2Enabled, poi3Latitude, poi3Longitude, poi3Name, poi3Enabled, theme, circle1, circle2, circle3]
            (const String& var) -> String {
                if (var == "LATITUDE")        return latitude;
                if (var == "LONGITUDE")       return longitude;
                if (var == "RADIUS")          return radius;
                if (var == "OPENSKY_ID")      return openskyClientId;
                if (var == "OPENSKY_SECRET")  return openskySecret;
                if (var == "CIRCLE1")         return circle1;
                if (var == "CIRCLE2")         return circle2;
                if (var == "CIRCLE3")         return circle3;
                if (var == "SCANLINE")        return scanlineEnabled == "true" ? "checked" : "";
                if (var == "INFOTEXT")        return infoTextEnabled == "true" ? "checked" : "";
                if (var == "TRIANGLE")        return triangleEnabled == "true" ? "checked" : "";
                if (var == "NORTH")           return northEnabled == "true" ? "checked" : "";
                if (var == "POI1_LATITUDE")   return poi1Latitude;
                if (var == "POI1_LONGITUDE")  return poi1Longitude;
                if (var == "POI1_NAME")       return poi1Name;
                if (var == "POI1_ENABLED")    return poi1Enabled == "true" ? "checked" : "";
                if (var == "POI2_LATITUDE")   return poi2Latitude;
                if (var == "POI2_LONGITUDE")  return poi2Longitude;
                if (var == "POI2_NAME")       return poi2Name;
                if (var == "POI2_ENABLED")    return poi2Enabled == "true" ? "checked" : "";
                if (var == "POI3_LATITUDE")   return poi3Latitude;
                if (var == "POI3_LONGITUDE")  return poi3Longitude;
                if (var == "POI3_NAME")       return poi3Name;
                if (var == "POI3_ENABLED")    return poi3Enabled == "true" ? "checked" : "";
                if (var == "THEME_GREEN")     return theme == "green" ? "selected" : "";
                if (var == "THEME_CYAN")      return theme == "cyan" ? "selected" : "";
                if (var == "THEME_AMBER")     return theme == "amber" ? "selected" : "";
                if (var == "THEME_ALTITUDE")  return theme == "altitude" ? "selected" : "";
                return "";
            }
        );
        request->send(response);
        }
    );

    server.on("/check-update", HTTP_GET, [&](AsyncWebServerRequest* request) {
        Serial.println("[GET] Checking GitHub update...");

        const GitHubReleaseInfo release = FetchLatestGithubRelease();
        if (!release.success) {
            request->send(500, "application/json", String("{\"success\":false,\"error\":\"") + release.errorMessage + "\"}");
            return;
        }

        const int cmp = CompareVersions(release.latestVersion, CurrentFirmwareVersion);
        const bool updateAvailable = cmp > 0;
        const String payload = String("{\"success\":true,\"updateAvailable\":") + (updateAvailable ? "true" : "false") +
            String(",\"currentVersion\":\"") + CurrentFirmwareVersion + String("\",\"latestVersion\":\"") + release.latestVersion + String("\"}");
        request->send(200, "application/json", payload);
    });

    server.on("/apply-update", HTTP_POST, [&](AsyncWebServerRequest* request) {
        Serial.println("[POST] Applying GitHub update...");

        const GitHubReleaseInfo release = FetchLatestGithubRelease();
        if (!release.success) {
            request->send(500, "text/plain", String("Update check failed: ") + release.errorMessage);
            return;
        }

        if (CompareVersions(release.latestVersion, CurrentFirmwareVersion) <= 0) {
            request->send(200, "text/plain", "Aucune mise à jour disponible.");
            return;
        }

        String errorMessage;
        if (!PerformGithubOTA(release.downloadUrl, errorMessage)) {
            request->send(500, "text/plain", String("OTA failed: ") + errorMessage);
            return;
        }

        request->send(200, "text/plain", String("Mise à jour appliquée vers ") + release.latestVersion + ". Redémarrage...");
        delay(100);
        ESP.restart();
    });

    // Handle save submission to web server
    server.on("/save", HTTP_POST, [&](AsyncWebServerRequest* request) {
        Serial.println("[POST] Handling form submission to config web server...");

        // safe parameter retrieval helper lambda
        auto TrySaveParam = [request, this](const char* paramName) {
            const auto* param = request->getParam(paramName, true);
            if (param == nullptr)
                return false;

            prefs.putString(paramName, param->value());
            return true;
            };

        prefs.begin("config", false);

        TrySaveParam("latitude");
        TrySaveParam("longitude");
        TrySaveParam("radius");
        TrySaveParam("circle1");
        TrySaveParam("circle2");
        TrySaveParam("circle3");
        TrySaveParam("poi1-latitude");
        TrySaveParam("poi1-longitude");
        TrySaveParam("poi1-name");
        TrySaveParam("poi2-latitude");
        TrySaveParam("poi2-longitude");
        TrySaveParam("poi2-name");
        TrySaveParam("poi3-latitude");
        TrySaveParam("poi3-longitude");
        TrySaveParam("poi3-name");
        prefs.putString("poi1-enabled", request->hasParam("poi1-enabled", true) ? "true" : "false");
        prefs.putString("poi2-enabled", request->hasParam("poi2-enabled", true) ? "true" : "false");
        prefs.putString("poi3-enabled", request->hasParam("poi3-enabled", true) ? "true" : "false");
        TrySaveParam("opensky-id");
        TrySaveParam("theme");

        const auto* param = request->getParam("opensky-secret", true);
        if (param != nullptr) {
            const String& secret = param->value();
            if (secret.indexOf('*') == -1) { // Special handling for secret: don't overwrite with masked value
                prefs.putString("opensky-secret", secret);
            }
        }

        prefs.putString("scanline", request->hasParam("scanline", true) ? "true" : "false");
        prefs.putString("triangle", request->hasParam("triangle", true) ? "true" : "false");
        prefs.putString("infotext", request->hasParam("infotext", true) ? "true" : "false");
        prefs.putString("north", request->hasParam("north", true) ? "true" : "false");
        prefs.end();

        request->send(200, "text/html", "Saved - restarting device...");
        ESP.restart();
        }
    );

    server.begin();
}

const String ConfigurationWebServer::GetStoredString(const char* key)
{
    prefs.begin("config", true);
    const String value = prefs.getString(key, "");
    prefs.end();
    return value;
}