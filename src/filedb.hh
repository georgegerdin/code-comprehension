/*
 * Copyright (c) 2022, Itamar S. <itamar8910@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <string_view>
#include <optional>

namespace CodeComprehension {

class FileDB {
    FileDB(const FileDB&) = delete;
    FileDB(FileDB&&) = delete;

public:
    virtual ~FileDB() = default;

    virtual std::optional<std::string> get_or_read_from_filesystem(std::string_view filename) const = 0;
    void set_project_root(std::optional<std::string_view> project_root)
    {
        if (!project_root.has_value())
            m_project_root.reset();
        else
            m_project_root = project_root;
    }
    std::optional<std::string> const& project_root() const { return m_project_root; }
    std::string to_absolute_path(std::string_view filename) const;

protected:
    FileDB() = default;

private:
    std::optional<std::string> m_project_root;
};

}
