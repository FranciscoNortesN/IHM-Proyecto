#ifndef MAPTOOLTYPES_H
#define MAPTOOLTYPES_H

#include <QString>

struct MapToolDescriptor
{
    QString id;
    QString title;
    QString resourcePath;
};

namespace MapToolMime
{
    inline constexpr const char *ToolId = "application/x-navtrainer-tool-id";
    inline constexpr const char *ToolSource = "application/x-navtrainer-tool-source";
}

#endif // MAPTOOLTYPES_H
