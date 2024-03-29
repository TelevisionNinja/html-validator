#include "htmlValidator.h"

std::unordered_set<std::string> voidElements = {
    "area",
    "base",
    "br",
    "col",
    "embed",
    "hr",
    "img",
    "input",
    "link",
    "meta",
    "param",
    "source",
    "track",
    "wbr",

    "command",
    "keygen",
    "menuitem",

    "basefont",
    "bgsound",
    "frame",
    "image",
    "isindex",
    "nextid",

    "!doctype"
};

// check for void elements
bool isVoidElement(std::string tag) {
    return voidElements.contains(tag);
}

std::string scriptTag = "script",
    styleTag = "style";

/**
 * check for valid HTML
 * 
 * @param {*} file the file name
 * @returns true if it is a valid file, false if it is not
 */
bool HTMLValidator(std::string file, bool enableErrorMessages) {
    std::ifstream din;
    din.open(file);

    // check for file error
    if (din.fail()) {
        if (enableErrorMessages) {
            std::cout << "The file failed to open\n";
        }
        return false;
    }

    //-----------------------------------------------------
    // set up variables

    std::stack<std::string> stack;
    // string to build current tag
    std::string tag = "";

    // here are boolean vars to detect the state of the iteration
    // boolean to ckeck if currently in a tag
    bool inTag = false,
        // boolean to ckeck if the tag is an end tag
        isEndingTag = false,
        // boolean to ckeck if the iteration is in a quote
        inString = false,
        // boolean to ckeck if the iteration is in a script
        inScript = false,
        // boolean to ckeck if the iteration is in CSS
        inCSS = false,
        // booleans to ckeck if the iteration is in a comment
        inComment = false,
        inMultiComment = false,
        inHTMLComment = false;

    // integer to keep track of the line number
    int lineNum = 1,
        // integer to keep track of the character number. starts at zero because when the end of a tag is detected, the char number will be at the detected tag ending char
        charIndex = 0;

    // char to keep track of the current character
    char ch = din.get(),
        // char to keep track of the previous character
        previous = '0',
        // char to keep track of the string starting char
        stringChar = '"';

    //-----------------------------------------------------
    // read file

    // iterate through all the chars
    while (ch != EOF) {
        // ignore case of the letters
        ch = tolower(ch);

        //-----------------------------------------------------
        // ignore scripts and CSS

        // ignore scripts
        if (!inHTMLComment && tag == scriptTag) {
            inScript = true;
        }

        // ignore CSS
        else if (!inHTMLComment && tag == styleTag) {
            inCSS = true;
        }

        bool isScriptOrCSS = inScript || inCSS;

        //-----------------------------------------------------
        // detect comments and strings

        // ignore multi line comments
        if (!inHTMLComment && isScriptOrCSS && !inString && !inComment && previous == '/' && ch == '*') {
            inMultiComment = true;
        }
        else if (!inHTMLComment && isScriptOrCSS && !inString && inMultiComment && previous == '*' && ch == '/') {
            inMultiComment = false;
        }

        // ignore single line comments
        else if (!inHTMLComment && isScriptOrCSS && !inString && !inMultiComment && previous == '/' && ch == '/') {
            inComment = true;
        }

        // ignore strings
        else if (!inHTMLComment && (ch == '"' || ch == '\'' || ch == '`')) {
            if (inString) {
                if (stringChar == ch) {
                    if ((ch == '`' && inScript) || (ch != '`' && isScriptOrCSS)) {
                        if (previous != '\\') {
                            inString = false;
                        }
                    }
                    else {
                        inString = false;
                    }
                }
            }
            else {
                if ((ch == '`' && inScript) || (ch != '`' && (isScriptOrCSS || inTag))) {
                    inString = true;
                    stringChar = ch;
                }
            }
        }

        //-----------------------------------------------------
        // keep track of line number
        else if (ch == '\n' || ch == '\r') {
            lineNum++;

            // reset character number when on a new line
            charIndex = 0;

            // reset comment detection
            inComment = false;
        }

        //-----------------------------------------------------
        // tags

        // detect start of a tag
        else if (!inHTMLComment && !inString && ch == '<') {
            inTag = true;
        }

        // check if the iteration is in a tag
        else if (!inHTMLComment && inTag) {
            // detect end of a tag
            if (ch == '>' || ch == ' ') {
                // push or pop tags
                if (isEndingTag) {
                    if (!stack.empty()) {
                        isEndingTag = false;
                        std::string check = stack.top();

                        // compare the tag with the stack to check for errors
                        if (tag == check) {
                            stack.pop();

                            // reset script and css states
                            inScript = false;
                            inCSS = false;
                        }
                        else {
                            // exit the validator
                            din.close();

                            // HTML nesting error
                            if (enableErrorMessages) {
                                std::cout << "Tag nesting error on line " << lineNum
                                    << " at character number " << charIndex - tag.size()
                                    << " for: </" << tag << ">\n"
                                    << "The expected tag was: </" << check << ">\n";
                            }

                            return false;
                        }
                    }
                    else {
                        // exit the validator
                        din.close();

                        // HTML nesting error
                        if (enableErrorMessages) {
                            std::cout << "Tag nesting error on line " << lineNum
                                << " at character number " << charIndex - tag.size() - 1
                                << "\nCannot start with an end tag\n";
                        }

                        return false;
                    }
                }
                else {
                    // filter out special tags, comments, scripts, and CSS
                    if ((!inScript && !inCSS && !isVoidElement(tag))
                        || (!inMultiComment && !inComment && (tag == scriptTag || tag == styleTag))) {
                        stack.push(tag);
                    }
                }

                // reset vars for the next tag
                tag = "";
                inTag = false;
            }
            // detect end tag
            else if (ch == '/') {
                isEndingTag = true;
            }
            // build tag
            else {
                tag += ch;

                // html comment
                if (tag.substr(0, 3) == "!--") {
                    inHTMLComment = true;
                }

                // ignore javascript and CSS
                else if (!isEndingTag && !inMultiComment && !inComment) {
                    if (inScript && scriptTag.substr(0, tag.length()) != tag) {
                        tag = "";
                        inTag = false;
                    }
                    else if (inCSS && styleTag.substr(0, tag.length()) != tag) {
                        tag = "";
                        inTag = false;
                    }
                }
            }
        }
        else if (inHTMLComment && inTag) {
            // detect end of a tag
            if (ch == '>') {
                if (tag.substr(tag.length() - 3) != "!--" && tag.substr(tag.length() - 2) == "--") {
                    // reset vars for the next tag
                    inHTMLComment = false;
                    tag = "";
                    inTag = false;
                }
            }
            // build tag
            else {
                tag += ch;
            }
        }

        //-----------------------------------------------------
        // read next char

        previous = ch;
        ch = din.get();

        // increment the char number in the current line
        charIndex++;
    }

    // close file
    din.close();

    // check for missing end tags
    if (stack.size()) {
        // print all missing end tags
        while (stack.size()) {
            if (enableErrorMessages) {
                std::cout << "Missing end tag for: <" << stack.top() << ">\n";
            }
            stack.pop();
        }

        return false;
    }

    if (enableErrorMessages) {
        std::cout << "This is a valid HTML file\n";
    }
    return true;
}
