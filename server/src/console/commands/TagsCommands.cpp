#include "console/commands/TagsCommands.hpp"

#include "Logger.hpp"
#include "console/ServerConsole.hpp"
#include "console/commands/CommandUtils.hpp"

void TagsCommands::handleCommand(ServerConsole* console, const std::string& cmd)
{
    std::string lowerCmd = CommandUtils::toLower(cmd);

    if (lowerCmd == "tags list") {
        handleList(console);
    } else if (CommandUtils::startsWithIgnoreCase(cmd, "tags add ")) {
        handleAdd(console, cmd.substr(9));
    } else if (CommandUtils::startsWithIgnoreCase(cmd, "tags remove ")) {
        handleRemove(console, cmd.substr(12));
    } else if (CommandUtils::startsWithIgnoreCase(cmd, "tags")) {
        handleHelp(console);
    }
}

void TagsCommands::handleList(ServerConsole* console)
{
    console->addAdminLog("[Tags] Enabled tags:");
    auto enabled = Logger::instance().getEnabledTags();
    if (enabled.empty()) {
        console->addAdminLog("  (none - showing all logs)");
    } else {
        std::string tags;
        for (const auto& tag : enabled) {
            tags += tag + " ";
        }
        console->addAdminLog("  " + tags);
    }
    console->addAdminLog("[Tags] Available tags:");
    auto all = Logger::instance().getAllKnownTags();
    std::string allTags;
    for (const auto& tag : all) {
        allTags += tag + " ";
    }
    console->addAdminLog("  " + allTags);
}

void TagsCommands::handleAdd(ServerConsole* console, const std::string& tagArg)
{
    std::string targetTag = tagArg;
    auto knownTags        = Logger::instance().getAllKnownTags();
    std::string lowerArg  = CommandUtils::toLower(tagArg);

    std::string cleanArg = lowerArg;
    if (cleanArg.size() >= 2 && cleanArg.front() == '[' && cleanArg.back() == ']') {
        cleanArg = cleanArg.substr(1, cleanArg.size() - 2);
    }

    for (const auto& known : knownTags) {
        std::string cleanKnown = CommandUtils::toLower(known);
        if (cleanKnown.size() >= 2 && cleanKnown.front() == '[' && cleanKnown.back() == ']') {
            cleanKnown = cleanKnown.substr(1, cleanKnown.size() - 2);
        }

        if (cleanKnown == cleanArg) {
            targetTag = known;
            break;
        }
    }

    Logger::instance().addTag(targetTag);
    console->addAdminLog("[Tags] Added tag: " + targetTag);
}

void TagsCommands::handleRemove(ServerConsole* console, const std::string& tagArg)
{
    std::string targetTag = tagArg;
    auto knownTags        = Logger::instance().getAllKnownTags();
    std::string lowerArg  = CommandUtils::toLower(tagArg);

    std::string cleanArg = lowerArg;
    if (cleanArg.size() >= 2 && cleanArg.front() == '[' && cleanArg.back() == ']') {
        cleanArg = cleanArg.substr(1, cleanArg.size() - 2);
    }

    for (const auto& known : knownTags) {
        std::string cleanKnown = CommandUtils::toLower(known);
        if (cleanKnown.size() >= 2 && cleanKnown.front() == '[' && cleanKnown.back() == ']') {
            cleanKnown = cleanKnown.substr(1, cleanKnown.size() - 2);
        }

        if (cleanKnown == cleanArg) {
            targetTag = known;
            break;
        }
    }

    Logger::instance().removeTag(targetTag);
    console->addAdminLog("[Tags] Removed tag: " + targetTag);
}

void TagsCommands::handleHelp(ServerConsole* console)
{
    console->addAdminLog("Usage:");
    console->addAdminLog("  tags list");
    console->addAdminLog("  tags add <tag>");
    console->addAdminLog("  tags remove <tag>");
}
