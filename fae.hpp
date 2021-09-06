#ifndef FAE_HPP
#define FAE_HPP

/**
 * @file fae.hpp
 * @brief Fae header file
 * @author Nick Muggio
 */

#include <any>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <iterator>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace fae
{
namespace detail
{
/**
 * @brief Type trait that identifies something a non-stream-insertable
 */
template <typename, typename = void>
struct IsStreamInsertable : std::false_type {};

/**
 * @brief Type trait that identifies something as stream-insertable
 */
template <typename T>
struct IsStreamInsertable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
: std::true_type {};

static_assert(IsStreamInsertable<int>::value);
static_assert(!IsStreamInsertable<std::variant<int>>::value);

/**
 * @brief Type trait that identifies something as a non-container
 */
template <typename, typename = void, typename = void>
struct IsContainer : std::false_type {};

/**
 * @brief Type trait that identifies something as a container, based on the presence of cbegin and cend methods
 * @todo This can probably be made more robust
 */
template <typename T>
struct IsContainer<T, std::void_t<decltype(std::declval<const T&>().cbegin())>, std::void_t<decltype(std::declval<const T&>().cend())>>
: std::true_type {};

static_assert(IsContainer<std::array<int, 5>>::value);
static_assert(IsContainer<std::deque<int>>::value);
static_assert(IsContainer<std::vector<int>>::value);
static_assert(!IsContainer<int>::value);
} // namespace detail

/**
 * @brief Used for exceptions that occur in fae
 */
using FaeException = std::runtime_error;

/**
 * @brief Convenience type for template inputs
 * @tparam T Variant types
 */
template <typename... T>
using Input = std::map<std::string, std::variant<T...>>;

class Library;

/**
 * @brief A parsed template
 */
class Template
{
  public:
    /**
     * @brief Default constructor
     */
    Template();

    /**
     * @brief Parse a template from memory
     * @param str String containing a template
     */
    explicit Template(const std::string& str);

    /**
     * @brief Render a template
     * @tparam T Variant types
     * @param input Input to the template. This is a map from std::string to a variant
     * @return Rendered template contents
     */
    template <typename... T>
    std::string render(const std::map<std::string, std::variant<T...>>& input);

  private:
    /**
     * @brief Render a template
     * @tparam T Variant types
     * @param input Input to the template. This is a map from std::string to a variant
     * @param pLibrary Pointer to the library (used when resolving includes)
     * @return Rendered template contents
     */
    template <typename... T>
    std::string libraryRender(const std::map<std::string, std::variant<T...>>& input, Library* pLibrary = nullptr);

    /**
     * @brief Render this template
     * @param rPrintFunc Function used to print a value.
     *        Must take the variable index as the first parameter, and the output stream as the second.
     * @param rKeyExistsFunc Function that returns true if the given key (specified as a variable index) exists, false otherwise
     * @param rNextListItem Function that takes advances to the next list item. First param is the single-item variable index.
     *        Second param is the list variable index. Should return true if another item was found in the list, false otherwise
     * @param rRenderInclude Render an included template. First param is template name ID, second is the output stream
     * @return Rendered template
     */
    std::string render(std::function<void(const std::uint16_t, std::ostringstream&)>&& rPrintFunc,
      std::function<bool(const std::uint16_t)>&& rKeyExistsFunc,
      std::function<bool(const std::uint16_t, const std::uint16_t)>&& rNextListItem,
      std::function<void(const std::uint16_t, std::ostringstream&)>&& rRenderInclude);

    /**
     * @brief Compile a template
     * @param buf Template to compile
     */
    void compile(const std::string& buf);

    /**
     * @brief Register a variable in the name list
     * @param name Variable name
     * @return Index of the variable in the list
     */
    std::uint16_t addVariable(const std::string& name);

    /**
     * @brief Add a fragment to the list, and the instruction to copy it
     * @param begin Start of the fragment
     * @param end One past the end of the fragment
     */
    void addFragment(const std::string::const_iterator& begin, const std::string::const_iterator& end);

    std::vector<std::string> m_fragments;    //!< Raw fragments from the template file
    std::vector<std::string> m_varNames;     //!< Indexed variable names
    std::vector<std::string> m_includeNames; //!< Names of included templates

    /**
     * @brief Valid opcodes for the template VM
     */
    enum Opcode : std::uint16_t
    {
      Halt = 0,
      Copy = 0x2000,
      Substitute = 0x4000,
      Immediate = 0x6000,
      FalseJump = 0x8000,
      ListEndJump = 0xA000,
      Jump = 0xC000,
      Include = 0xE000,
    };

    std::vector<std::uint16_t> m_bytecode; //!< Bytecode for this VM

    friend Library; //!< Allow Library to access libraryRender method
};

/**
 * @brief Contains a collection of Fae templates
 */
class Library
{
  public:
    /**
     * @brief Default constructor
     */
    Library();

    /**
     * @brief Build a library from a directory
     * @param directory Directory to assemble the library from
     * @param recursive Whether to search recursively for Fae template files
     * @param ignoreBadTemplates If true, invalid template files will be silently ignored. If false, an exception will
     *        be thrown if an invalid template is encountered. Defaults to true
     */
    Library(const std::filesystem::path& directory, bool recursive, bool ignoreBadTemplates = true);

    /**
     * @brief Render a template
     * @tparam T Variant types
     * @param templateName Name of the template to render
     * @param input Input to the template. This is a map from std::string to a variant
     * @throws Throws a FaeException if the specified template is not in the library
     * @return Rendered template contents
     */
    template <typename... T>
    std::string render(const std::string& templateName, const std::map<std::string, std::variant<T...>>& input);

    /**
     * @brief Reload the template library from disk
     * @param discard If true, existing templates will be discarded before reading new templates
     */
    void reload(bool discard = true);

  private:
    /**
     * @brief Load and compile a template, and add it to the library
     * @param file Path to the template file
     */
    void addTemplate(const std::filesystem::path& file);

    std::filesystem::path m_directory;
    bool m_recursive;
    bool m_ignoreBadTemplates;

    std::map<std::string, Template> m_templates; //!< Name/template map
};

template <typename... T>
std::string Template::render(const std::map<std::string, std::variant<T...>>& input)
{
  return libraryRender(input);
}

template <typename... T>
std::string Template::libraryRender(const std::map<std::string, std::variant<T...>>& input, Library* pLibrary)
{
  // Map from single variable indices to {iterator pointing to container, iterator into container}
  // TODO move away from std::any
  using TupleType = std::tuple<decltype(std::declval<const decltype(input)&>().cbegin()), std::any>;
  std::map<std::uint16_t, TupleType> currentIterators;

  auto printVar = [&](const std::uint16_t key, std::ostringstream& rOutStream) -> void
    {
      auto curIter = currentIterators.find(key);
      if (curIter != currentIterators.end())
      {
        std::visit([&](auto& rContainer)
        {
          if constexpr (detail::IsContainer<decltype(rContainer)>::value)
          {
            // We have a container!
            using IterType = decltype(rContainer.cbegin());
            using ValType = decltype(*rContainer.cbegin());

            if constexpr (detail::IsStreamInsertable<ValType>::value)
            {
              rOutStream << *std::any_cast<IterType>(std::get<1>(curIter->second));
            }
          }
        }, std::get<0>(curIter->second)->second);
      }
      else
      {
        auto varIter = input.find(m_varNames[key]);
        if (varIter != input.end())
        {
          std::visit([&](auto& rArg)
            {
              if constexpr (detail::IsStreamInsertable<decltype(rArg)>::value)
              {
                rOutStream << rArg;
              }
            }, varIter->second);
        }
      }
    };

  auto varExists = [&](const std::uint16_t key) -> bool
    {
      return input.find(m_varNames[key]) != input.end() || currentIterators.find(key) != currentIterators.end();
    };

  auto advanceContainer = [&](const std::uint16_t singleIdx, const std::uint16_t listIdx) -> bool
    {
      auto containerPair = input.find(m_varNames[listIdx]);

      if (containerPair == input.end())
      {
        return false;
      }

      return std::visit([&](auto& rContainer) -> bool
      {
        if constexpr (detail::IsContainer<decltype(rContainer)>::value)
        {
          using ContainerType = typename std::remove_reference<decltype(rContainer)>::type;
          using IterType = decltype(rContainer.cbegin());

          if (std::distance(rContainer.cbegin(), rContainer.cend()) == 0)
          {
            return false;
          }

          auto curPair = currentIterators.find(singleIdx);

          if (curPair == currentIterators.end())
          {
            // Not in the current iterator map, so let's add it
            currentIterators[singleIdx] = TupleType{containerPair, rContainer.cbegin()};

            // We know the container isn't empty, and we just started, so there is at least one element left
            return true;
          }
          else
          {
            // Single-variable is already in the map. Advance and check if we're at the end
            auto& rIter = std::any_cast<IterType&>(std::get<1>(curPair->second));
            ++rIter;

            if (rIter == rContainer.cend())
            {
              // Remove from the map
              currentIterators.erase(curPair);
              return false;
            }

            return true;
          }
        }

        return false;
      }, containerPair->second);
    };

    auto renderInclude = [&](std::uint16_t id, std::ostringstream& rOutStream)
      {
        // TODO include failures be loud? Maybe configurable?
        if (pLibrary != nullptr)
        {
          try
          {
            rOutStream << pLibrary->render(m_includeNames[id], input);
          }
          catch (const FaeException&)
          {
            // Ignored
          }
        }
      };

  return render(printVar, varExists, advanceContainer, renderInclude);
}

template <typename... T>
std::string Library::render(const std::string& templateName, const std::map<std::string, std::variant<T...>>& input)
{
  auto iter = m_templates.find(templateName);
  if (iter != m_templates.end())
  {
    return iter->second.libraryRender(input, this);
  }

  std::stringstream stream;
  stream << "Could not find a template named '" << templateName << '\'';
  throw FaeException(stream.str());
}
} // namespace fae

#endif // FAE_HPP
