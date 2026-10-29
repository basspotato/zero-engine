#include "Freesound.h"
#include "../../Source/Parameters.h"
#include <set>

namespace
{
    const juce::String apiRoot ("https://freesound.org/apiv2/");
    constexpr int callbackPort = 8765;

    juce::String env (const char* name)
    {
        return juce::SystemStats::getEnvironmentVariable (name, {}).trim();
    }

    // Search words per genre and type, as people tag them on Freesound.
    juce::String queryFor (int genre, int type)
    {
        const char* genreWords[] { "edm", "dubstep", "techno", "tech house", "trap", "synthwave",
                                   "dnb", "pop", "house", "trance", "future bass", "hardstyle" };
        const char* typeWords[]  { "bass", "lead", "chord", "pad", "pluck", "fx" };

        if (genreNames[genre] == "Trap" && typeNames[type] == "Bass")
            return "808";

        return juce::String (genreWords[genre]) + " " + typeWords[type];
    }

    juce::String get (const juce::URL& url, const juce::String& headers, int& status)
    {
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (20000)
                           .withExtraHeaders (headers)
                           .withStatusCode (&status);
        auto stream = url.createInputStream (options);
        return stream != nullptr ? stream->readEntireStreamAsString() : juce::String();
    }

    juce::String csvField (const juce::String& s)
    {
        return "\"" + s.replace ("\"", "\"\"") + "\"";
    }

    juce::StringArray parseCsvLine (const juce::String& line)
    {
        juce::StringArray fields;
        juce::String current;
        bool quoted = false;

        for (int i = 0; i < line.length(); ++i)
        {
            const auto c = line[i];
            if (quoted)
            {
                if (c == '"' && line[i + 1] == '"') { current << '"'; ++i; }
                else if (c == '"')                  quoted = false;
                else                                current << c;
            }
            else if (c == '"') quoted = true;
            else if (c == ',') { fields.add (current); current.clear(); }
            else               current << c;
        }

        fields.add (current);
        return fields;
    }

    //==========================================================================
    // OAuth2: the owner approves in their browser; Freesound redirects to a
    // one-shot listener on localhost with a code, which is swapped for a token.
    juce::String authorise (const juce::String& clientId, const juce::String& secret)
    {
        juce::StreamingSocket server;
        if (! server.createListener (callbackPort, "127.0.0.1"))
        {
            std::printf ("Couldn't listen on port %d (is something else using it?)\n", callbackPort);
            return {};
        }

        const auto state = juce::String::toHexString (juce::Random::getSystemRandom().nextInt64());
        juce::URL (apiRoot + "oauth2/authorize/")
            .withParameter ("client_id", clientId)
            .withParameter ("response_type", "code")
            .withParameter ("state", state)
            .launchInDefaultBrowser();

        std::printf ("Approve access in the browser window that just opened (waiting)...\n");

        juce::String code;
        while (code.isEmpty())
        {
            std::unique_ptr<juce::StreamingSocket> client (server.waitForNextConnection());
            if (client == nullptr)
                return {};

            char buffer[4096] {};
            if (client->waitUntilReady (true, 5000) == 1)
                client->read (buffer, (int) sizeof (buffer) - 1, false);

            // "GET /callback?code=...&state=... HTTP/1.1"
            const auto target = juce::String (buffer).upToFirstOccurrenceOf ("\r\n", false, false)
                                    .fromFirstOccurrenceOf ("GET ", false, false)
                                    .upToFirstOccurrenceOf (" ", false, false);
            const juce::URL callback ("http://localhost" + target);
            const auto names = callback.getParameterNames();
            const auto values = callback.getParameterValues();

            const auto ok = values[names.indexOf ("state")] == state && names.contains ("code");
            if (ok)
                code = values[names.indexOf ("code")];

            const juce::String body = ok ? "Zero Engine: access approved. You can close this tab."
                                         : "Zero Engine: waiting for the Freesound approval.";
            const auto response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
                                + juce::String (body.getNumBytesAsUTF8()) + "\r\nConnection: close\r\n\r\n" + body;
            client->write (response.toRawUTF8(), (int) response.getNumBytesAsUTF8());
        }

        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData).withConnectionTimeoutMs (20000);
        auto stream = juce::URL (apiRoot + "oauth2/access_token/")
                          .withParameter ("client_id", clientId)
                          .withParameter ("client_secret", secret)
                          .withParameter ("grant_type", "authorization_code")
                          .withParameter ("code", code)
                          .createInputStream (options);

        const auto json = juce::JSON::parse (stream != nullptr ? stream->readEntireStreamAsString() : juce::String());
        return json.getProperty ("access_token", {}).toString();
    }
}

//==============================================================================
int screenFreesound (const juce::File& shortlist, int perPair)
{
    const auto secret = env ("FREESOUND_CLIENT_SECRET");
    if (secret.isEmpty())
    {
        std::printf ("Set FREESOUND_CLIENT_SECRET first (your Freesound API key).\n");
        return 1;
    }

    juce::String csv ("genre,type,id,name,username,license,duration,downloads,rating,format,url\n");
    std::set<int> seen;
    int total = 0;

    for (int g = 0; g < genreNames.size(); ++g)
    {
        for (int t = 0; t < typeNames.size(); ++t)
        {
            const auto url = juce::URL (apiRoot + "search/text/")
                                 .withParameter ("query", queryFor (g, t))
                                 .withParameter ("filter", "license:\"Creative Commons 0\" duration:[0.1 TO 15]")
                                 .withParameter ("fields", "id,name,username,license,duration,num_downloads,avg_rating,type,url")
                                 .withParameter ("sort", "downloads_desc")
                                 // Ask for extra so sounds already listed under another pair don't starve this one.
                                 .withParameter ("page_size", juce::String (juce::jlimit (1, 150, perPair * 4)))
                                 .withParameter ("token", secret);

            int status = 0;
            const auto json = juce::JSON::parse (get (url, {}, status));
            if (status != 200)
            {
                std::printf ("Search failed (HTTP %d) for \"%s\"; check the API key.\n", status, queryFor (g, t).toRawUTF8());
                return 1;
            }

            int kept = 0;
            if (auto* results = json.getProperty ("results", {}).getArray())
            {
                for (const auto& r : *results)
                {
                    if (kept >= perPair)
                        break;

                    const auto id = (int) r.getProperty ("id", 0);
                    if (! seen.insert (id).second)
                        continue; // already listed under another pair

                    csv << csvField (genreNames[g]) << "," << csvField (typeNames[t]) << "," << id << ","
                        << csvField (r.getProperty ("name", {}).toString()) << ","
                        << csvField (r.getProperty ("username", {}).toString()) << ","
                        << csvField (r.getProperty ("license", {}).toString()) << ","
                        << juce::String ((double) r.getProperty ("duration", 0.0), 2) << ","
                        << (int) r.getProperty ("num_downloads", 0) << ","
                        << juce::String ((double) r.getProperty ("avg_rating", 0.0), 1) << ","
                        << r.getProperty ("type", {}).toString() << ","
                        << csvField (r.getProperty ("url", {}).toString()) << "\n";
                    ++kept;
                }
            }

            std::printf ("  %-26s %3d  (\"%s\", %d CC0 matches)\n", (genreNames[g] + " / " + typeNames[t]).toRawUTF8(),
                         kept, queryFor (g, t).toRawUTF8(), (int) json.getProperty ("count", 0));
            total += kept;
            juce::Thread::sleep (1100); // stay under ~60 requests a minute
        }
    }

    shortlist.getParentDirectory().createDirectory();
    shortlist.replaceWithText (csv);
    std::printf ("Shortlisted %d sounds: %s\nDelete rows you don't want, then run fetch.\n", total, shortlist.getFullPathName().toRawUTF8());
    return 0;
}

int screenFreesoundForMatch (const juce::File& shortlist, int perQuery)
{
    // Synth sounds of every kind for testing Reference Match: by type, not
    // genre, CC0 only, no drums, percussion or voices. Genre is "Match" so
    // fetch files them under Match/<Type>.
    const auto secret = env ("FREESOUND_CLIENT_SECRET");
    if (secret.isEmpty())
    {
        std::printf ("Set FREESOUND_CLIENT_SECRET first (your Freesound API key).\n");
        return 1;
    }

    const std::pair<const char*, const char*> queries[] {
        { "Bass", "synth bass" },       { "Bass", "reese bass" },       { "Bass", "808 bass" },        { "Bass", "sub bass synth" },
        { "Bass", "wobble bass" },      { "Bass", "acid bass 303" },    { "Bass", "bass one shot" },
        { "Lead", "synth lead" },       { "Lead", "supersaw" },         { "Lead", "saw lead" },        { "Lead", "square lead synth" },
        { "Lead", "analog synth lead" },
        { "Chord", "synth chord" },     { "Chord", "stab chord" },      { "Chord", "chord stab synth" },
        { "Pad", "synth pad" },         { "Pad", "analog pad" },        { "Pad", "warm pad" },         { "Pad", "string synth" },
        { "Pluck", "synth pluck" },     { "Pluck", "pluck one shot" },  { "Pluck", "bell synth" },     { "Pluck", "arp loop synth" },
        { "Pluck", "arpeggio synth" },  { "Pluck", "synth sequence loop" },
        { "FX", "riser synth" },        { "FX", "synth sweep" },        { "FX", "downlifter" },        { "FX", "laser synth" },
    };

    juce::String csv ("genre,type,id,name,username,license,duration,downloads,rating,format,url\n");
    std::set<int> seen;
    int total = 0;
    for (const auto& [type, query] : queries)
    {
        const auto url = juce::URL (apiRoot + "search/text/")
                             .withParameter ("query", query)
                             .withParameter ("filter", "license:\"Creative Commons 0\" duration:[0.2 TO 10] "
                                                       "-tag:drum -tag:drums -tag:percussion -tag:kick -tag:snare -tag:hihat "
                                                       "-tag:vocal -tag:voice -tag:speech -tag:field-recording")
                             .withParameter ("fields", "id,name,username,license,duration,num_downloads,avg_rating,type,url")
                             .withParameter ("sort", "downloads_desc")
                             .withParameter ("page_size", juce::String (juce::jlimit (1, 150, perQuery * 2)))
                             .withParameter ("token", secret);

        int status = 0;
        const auto json = juce::JSON::parse (get (url, {}, status));
        if (status != 200)
        {
            std::printf ("Search failed (HTTP %d) for \"%s\"; check the API key.\n", status, query);
            return 1;
        }

        int kept = 0;
        if (auto* results = json.getProperty ("results", {}).getArray())
            for (const auto& r : *results)
            {
                if (kept >= perQuery)
                    break;
                const auto id = (int) r.getProperty ("id", 0);
                if (! seen.insert (id).second)
                    continue;
                csv << csvField ("Match") << "," << csvField (type) << "," << id << ","
                    << csvField (r.getProperty ("name", {}).toString()) << ","
                    << csvField (r.getProperty ("username", {}).toString()) << ","
                    << csvField (r.getProperty ("license", {}).toString()) << ","
                    << juce::String ((double) r.getProperty ("duration", 0.0), 2) << ","
                    << (int) r.getProperty ("num_downloads", 0) << ","
                    << juce::String ((double) r.getProperty ("avg_rating", 0.0), 1) << ","
                    << r.getProperty ("type", {}).toString() << ","
                    << csvField (r.getProperty ("url", {}).toString()) << "\n";
                ++kept;
            }

        std::printf ("  %-6s %-22s %3d  (%d CC0 matches)\n", type, query, kept, (int) json.getProperty ("count", 0));
        total += kept;
        juce::Thread::sleep (1100); // stay under ~60 requests a minute
    }

    shortlist.getParentDirectory().createDirectory();
    shortlist.replaceWithText (csv);
    std::printf ("Shortlisted %d sounds: %s\nThen: ZeroAnalyze fetch %s analysis/library/freesound\n", total,
                 shortlist.getFullPathName().toRawUTF8(), shortlist.getFullPathName().toRawUTF8());
    return 0;
}

int fetchFreesound (const juce::File& shortlist, const juce::File& outDir, int maxDownloads)
{
    const auto clientId = env ("FREESOUND_CLIENT_ID");
    const auto secret = env ("FREESOUND_CLIENT_SECRET");
    if (clientId.isEmpty() || secret.isEmpty())
    {
        std::printf ("Set FREESOUND_CLIENT_ID and FREESOUND_CLIENT_SECRET first.\n");
        return 1;
    }

    juce::StringArray lines;
    shortlist.readLines (lines);
    lines.removeEmptyStrings();
    if (lines.size() < 2)
    {
        std::printf ("Shortlist is empty or missing: %s (run screen first)\n", shortlist.getFullPathName().toRawUTF8());
        return 1;
    }

    const auto header = parseCsvLine (lines[0]);
    auto column = [&] (const juce::StringArray& row, const char* name) { return row[header.indexOf (name)]; };

    const auto token = authorise (clientId, secret);
    if (token.isEmpty())
    {
        std::printf ("Freesound didn't grant access.\n");
        return 1;
    }

    const auto creditsFile = outDir.getChildFile ("credits.csv");
    outDir.createDirectory();
    if (! creditsFile.existsAsFile())
        creditsFile.replaceWithText ("genre,type,id,name,username,license,url,file\n");

    int downloaded = 0, skipped = 0;
    for (int i = 1; i < lines.size() && downloaded < maxDownloads; ++i)
    {
        const auto row = parseCsvLine (lines[i]);
        const auto id = column (row, "id");
        const auto license = column (row, "license");

        // CC0 only, even if the CSV was edited (Freesound gives the licence as a URL).
        if (! (license.containsIgnoreCase ("publicdomain/zero") || license.containsIgnoreCase ("Creative Commons 0")))
        {
            std::printf ("  skip %s: not CC0 (%s)\n", id.toRawUTF8(), license.toRawUTF8());
            continue;
        }

        const auto folder = outDir.getChildFile (juce::File::createLegalFileName (column (row, "genre")))
                                  .getChildFile (juce::File::createLegalFileName (column (row, "type")));
        const auto name = juce::File::createLegalFileName (column (row, "name").upToLastOccurrenceOf (".", false, false)
                                                               .substring (0, 60));
        const auto file = folder.getChildFile (name + "_" + id + "." + column (row, "format"));

        if (file.existsAsFile())
        {
            ++skipped;
            continue;
        }

        folder.createDirectory();
        int status = 0;
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (30000)
                           .withExtraHeaders ("Authorization: Bearer " + token)
                           .withStatusCode (&status);
        auto stream = juce::URL (apiRoot + "sounds/" + id + "/download/").createInputStream (options);

        if (stream == nullptr || status != 200)
        {
            std::printf ("  failed %s (HTTP %d)\n", id.toRawUTF8(), status);
            if (status == 429)
            {
                std::printf ("Freesound's rate limit reached; run fetch again later to continue.\n");
                break;
            }
            continue;
        }

        {
            juce::FileOutputStream out (file);
            out.writeFromInputStream (*stream, -1);
        }

        creditsFile.appendText (csvField (column (row, "genre")) + "," + csvField (column (row, "type")) + "," + id + ","
                                + csvField (column (row, "name")) + "," + csvField (column (row, "username")) + ","
                                + csvField (license) + "," + csvField (column (row, "url")) + ","
                                + csvField (file.getRelativePathFrom (outDir)) + "\n");

        ++downloaded;
        std::printf ("  %d  %s\n", downloaded, file.getRelativePathFrom (outDir).toRawUTF8());
        juce::Thread::sleep (1100);
    }

    std::printf ("Downloaded %d, already had %d. Credits: %s\n", downloaded, skipped, creditsFile.getFullPathName().toRawUTF8());
    return 0;
}
