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
    "nextid"
};

// check for void elements
bool isVoidElement(std::string tag) {
    return voidElements.count(tag);
}

/**
 * check for valid HTML
 * 
 * @param {*} file the file name
 * @returns 
 */
void HTMLValidator(std::string file) {
    std::ifstream din;
    din.open(file);

    // check for file error
    if (din.fail()) {
        std::cout << "The file failed to open\n";
        return;
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
        inMultiComment = false;

    // integer to keep track of the line number
    int lineNum = 1,
        // integer to keep track of the character number
        charIndex = 0;

    // char to keep track of the current character
    char ch = din.get(),
        // char to keep track of the previous character
        previous = '0';

    //-----------------------------------------------------
    // read file

    // iterate through all the chars
    while (ch != EOF) {
        // ignore case of the letters
        ch = tolower(ch);

        // iterate the char number in the current line
        charIndex++;

        //-----------------------------------------------------
        // ignore scripts and CSS

        // ignore scripts
        if (tag == "script") {
            inScript = true;
        }

        // ignore CSS
        else if (tag == "style") {
            inCSS = true;
        }

        bool isScriptOrCSS = inScript || inCSS;

        //-----------------------------------------------------
        // detect comments and strings

        // ignore multi line comments
        if (previous == '/' && ch == '*' && isScriptOrCSS) {
            inMultiComment = true;
        }
        else if (inMultiComment && previous == '*' && ch == '/' && isScriptOrCSS) {
            inMultiComment = false;
        }

        // ignore comments
        else if (previous == '/' && ch == '/' && isScriptOrCSS) {
            inComment = true;
        }

        // ignore strings
        else if (ch == '"') {
            inString = !inString;
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
        else if (ch == '<' && !inString) {
            inTag = true;
        }

        // check if it is in a tag
        else if (inTag) {
            // detect end of a tag
            if (ch == '>' || ch == ' ') {
                // push or pop tags
                if (isEndingTag) {
                    isEndingTag = false;
                    std::string check;
                    check = stack.top();

                    // compare the tag with the stack to check for errors
                    if (tag == check) {
                        stack.pop();

                        // reset special cases
                        inScript = false;
                        inCSS = false;
                    }
                    else {
                        // HTML nesting error
                        std::cout << "Tag nesting error on line " << lineNum
                            << " at character number " << charIndex
                            << " for: </" << tag << ">\n"
                            << "The expected tag was: </" << check << ">\n";

                        // end function
                        din.close();
                        return;
                    }
                }
                else {
                    // filter out special tags, comments, scripts, and CSS
                    if ((!isVoidElement(tag) && tag.substr(0, 3) != "!--" && !inScript && !inCSS)
                        || ((tag == "script" || tag == "style") && (!inMultiComment && !inComment))) {
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

                // ignore javascript and CSS
                std::string scriptTag = "script",
                    styleTag = "style";

                if (!isEndingTag && !inMultiComment && !inComment) {
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

        //-----------------------------------------------------
        // read next char

        previous = ch;
        ch = din.get();
    }

    // check for missing end tags
    if (stack.size()) {
        // print all missing end tags
        while (stack.size()) {
            std::string top = stack.top();
            stack.pop();
            std::cout << "Missing end tag for: <" << top << ">\n";
        }
    }
    else {
        std::cout << "This is a valid HTML file\n";
    }

    // close file
    din.close();
}
