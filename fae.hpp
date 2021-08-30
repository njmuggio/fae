#ifndef FAE_HPP
#define FAE_HPP

/**
 * @file fae.hpp
 * @brief Fae header file
 * @author Nick Muggio
 */

#include <filesystem>
#include <map>
#include <string>

namespace fae
{
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
    Template(const std::string& str);

    /**
     * @brief Parse a template from a file
     * @param file File containing a template
     */
    Template(const std::filesystem::path& file);
};

/**
 * @brief Contains a collection of fae templates
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
     * @param recursive Whether to search recursively for fae template files
     * @param ignoreBadTemplates If true, invalid template files will be silently ignored. If false, an exception will
     *        be thrown if an invalid template is encountered. Defaults to true
     */
    Library(const std::filesystem::path& directory, bool recursive, bool ignoreBadTemplates = true);

  private:
    std::map<std::string, Template> m_templates; //!< Name/template map
};
} // namespace fae

#endif // FAE_HPP
