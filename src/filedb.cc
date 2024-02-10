/*
 * Copyright (c) 2022, Itamar S. <itamar8910@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <filesystem>
#include <fmt/format.h>
#include "filedb.hh"

namespace CodeComprehension {

std::string FileDB::to_absolute_path(std::string_view filename) const
{
    if (std::filesystem::path { filename }.is_absolute()) {
        return std::string{filename};
    }
    if (!m_project_root.has_value())
        return std::string{filename};
    return std::filesystem::path { fmt::format("{}/{}", *m_project_root, filename) }.string();
}

}