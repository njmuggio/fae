/**
 * @file fae.cpp
 * @brief Fae implementation
 * @author Nick Muggio
 */

#include <fstream>
#include <ios>
#include <regex>
#include <stack>
#include <stdexcept>

#include "fae.hpp"

namespace fae
{
Template::Template()
  : m_fragments(),
    m_varNames(),
    m_includeNames(),
    m_bytecode()
{
  compile("");
}

Template::Template(const std::string& str)
  : m_fragments(),
    m_varNames(),
    m_includeNames(),
    m_bytecode()
{
  compile(str);
}

std::string Template::render(std::function<void(const std::uint16_t, std::ostringstream&)>&& rPrintFunc,
  std::function<bool(const std::uint16_t)>&& rKeyExistsFunc,
  std::function<bool(const std::uint16_t, const std::uint16_t)>&& rNextListItem,
  std::function<void(const std::uint16_t, std::ostringstream&)>&& rRenderInclude)
{
  std::ostringstream stream;

  stream << std::boolalpha;

  for (size_t pc = 0; pc < m_bytecode.size(); pc++)
  {
    const std::uint16_t op = m_bytecode[pc];

    if (op == Halt)
    {
      break;
    }

    switch (op & 0xE000)
    {
      case Copy:
        stream << m_fragments[op & 0x1FFF];
        break;
      case Substitute:
        rPrintFunc(op & 0x1FFF, stream);
        break;
      case Immediate:
        // Nothing to do
        break;
      case FalseJump:
      {
        std::uint16_t varIdx = m_bytecode[pc - 1] & 0x1FFF;
        if (!rKeyExistsFunc(varIdx))
        {
          pc = (m_bytecode[pc] & 0x1FFF) - 1;
        }
        break;
      }
      case ListEndJump:
      {
        std::uint16_t singleIdx = m_bytecode[pc - 2] & 0x1FFF;
        std::uint16_t listIdx = m_bytecode[pc - 1] & 0x1FFF;
        if (!rNextListItem(singleIdx, listIdx))
        {
          pc = (m_bytecode[pc] & 0x1FFF) - 1;
        }
        break;
      }
      case Jump:
        pc = (m_bytecode[pc] & 0x1FFF) - 1;
        break;
      case Include:
        rRenderInclude(op & 0x1FFF, stream);
        break;
      default:
        throw FaeException("Unrecognized instruction encountered in template VM");
    }
  }

  return stream.str();
}

void Template::compile(const std::string& buf)
{
  size_t processed = 0;

  std::regex varSubExp(R"(^([a-zA-Z_][a-zA-Z0-9_]*)\))");
  std::regex ifExp(R"(^if\s+([a-zA-Z_][a-zA-Z0-9_]*)\))");
  std::regex endExp("^end\\)");
  std::regex forExp(R"(^for\s++([a-zA-Z_][a-zA-Z0-9_]*)\s++in\s++([a-zA-Z_][a-zA-Z0-9_]*)\))");
  std::regex includeExp(R"(^include ([^)]+)\))");

  std::smatch matches;

  std::stack<std::uint16_t> offsetStack;

  while (processed < buf.size())
  {
    // Find the next possible expression start
    size_t expStart = buf.find("$(", processed);

    if (expStart == std::string::npos)
    {
      m_fragments.emplace_back(buf.begin() + processed, buf.end());
      m_bytecode.push_back(Opcode::Copy | (m_fragments.size() - 1));
      processed = buf.size();
      break;
    }

    if (expStart - 1 >= 0 && buf[expStart - 1] == '\\')
    {
      // Might be escaped
      if (expStart - 2 >= 0 && buf[expStart - 2] == '\\')
      {
        // Escape was escaped - add fragment
        addFragment(buf.begin() + processed, buf.begin() + expStart - 1);
        processed = expStart;
      }
      else
      {
        addFragment(buf.begin() + processed, buf.begin() + expStart - 1);
        m_fragments.back() += '$';

        processed = expStart + 1;
        continue;
      }
    }

    if (expStart - processed > 1)
    {
      m_fragments.emplace_back(buf.begin() + processed, buf.begin() + expStart);
      m_bytecode.push_back(Opcode::Copy | (m_fragments.size() - 1));
    }

    if (std::regex_search(buf.begin() + expStart + 2, buf.end(), matches, endExp))
    {
      // Found an end
      if ((m_bytecode[offsetStack.top()] & 0xE000) == Opcode::ListEndJump)
      {
        // Need to jump back to start of list
        m_bytecode.push_back(Opcode::Jump | offsetStack.top());
      }

      auto nextIndex = m_bytecode.size();
      m_bytecode[offsetStack.top()] |= nextIndex;

      offsetStack.pop();
    }
    else if (std::regex_search(buf.begin() + expStart + 2, buf.end(), matches, varSubExp))
    {
      // Found a variable substitution
      std::uint16_t idx = addVariable(matches[1]);
      m_bytecode.push_back(Opcode::Substitute | idx);
    }
    else if (std::regex_search(buf.begin() + expStart + 2, buf.end(), matches, ifExp))
    {
      // Found an if

      // Push variable index
      std::uint16_t idx = addVariable(matches[1]);
      m_bytecode.push_back(Opcode::Immediate | idx);

      // If variable evals to false, jump to first instruction after conditional block
      m_bytecode.push_back(Opcode::FalseJump); // Address updated once 'end' is found
      offsetStack.push(m_bytecode.size() - 1);
    }
    else if (std::regex_search(buf.begin() + expStart + 2, buf.end(), matches, forExp))
    {
      // Found a for

      // Push single-value variable index
      std::uint16_t idx = addVariable(matches[1]);
      m_bytecode.push_back(Opcode::Immediate | idx);

      // Push list variable index
      idx = addVariable(matches[2]);
      m_bytecode.push_back(Opcode::Immediate | idx);

      // Once list is empty, jump to first instruction after conditional block
      m_bytecode.push_back(Opcode::ListEndJump);
      offsetStack.push(m_bytecode.size() - 1);
    }
    else if (std::regex_search(buf.begin() + expStart + 2, buf.end(), matches, includeExp))
    {
      // Found an include
      m_bytecode.push_back(Opcode::Include | m_includeNames.size());
      m_includeNames.push_back(matches[1]);
    }
    else
    {
      throw FaeException("Invalid template");
    }

    processed = expStart + 2 + matches[0].length();
  }

  m_bytecode.push_back(Opcode::Halt);
}

std::uint16_t Template::addVariable(const std::string& name)
{
  for (std::uint16_t i = 0; i < m_varNames.size(); i++)
  {
    if (m_varNames[i] == name)
    {
      return i;
    }
  }

  m_varNames.push_back(name);
  return m_varNames.size() - 1;
}

void Template::addFragment(const std::string::const_iterator& begin, const std::string::const_iterator& end)
{
  m_fragments.emplace_back(begin, end);
  m_bytecode.push_back(Opcode::Copy | (m_fragments.size() - 1));
}

Library::Library()
  : m_directory(),
    m_recursive(),
    m_ignoreBadTemplates(),
    m_templates()
{
  // Empty
}

Library::Library(const std::filesystem::path& directory, bool recursive, bool ignoreBadTemplates)
  : m_directory(directory),
    m_recursive(recursive),
    m_ignoreBadTemplates(ignoreBadTemplates),
    m_templates()
{
  reload(false);
}

void Library::reload(bool discard)
{
  if (discard)
  {
    m_templates.clear();
  }

  if (m_recursive)
  {
    for (const auto& file : std::filesystem::recursive_directory_iterator(m_directory))
    {
      addTemplate(file);
    }
  }
  else
  {
    for (const auto& file : std::filesystem::directory_iterator(m_directory))
    {
      addTemplate(file);
    }
  }
}

void Library::addTemplate(const std::filesystem::path& file)
{
  if (!std::filesystem::is_regular_file(file))
  {
    return;
  }

  std::ifstream stream(file, std::ios::binary);
  stream.seekg(0, std::ios::end);
  std::string buf(stream.tellg(), 0);
  stream.seekg(0);
  stream.read(buf.data(), buf.size());
  stream.close();

  if (m_ignoreBadTemplates)
  {
    try
    {
      m_templates[file.lexically_relative(m_directory)] = Template(buf);
    }
    catch (const FaeException&)
    {
      // Ignored
    }
  }
  else
  {
    m_templates[file.lexically_relative(m_directory)] = Template(buf);
  }
}
} // namespace fae
