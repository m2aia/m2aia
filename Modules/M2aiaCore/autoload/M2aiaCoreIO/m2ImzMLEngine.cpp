/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#include <m2ImzMLEngine.h>

std::string m2::TemplateEngine::render(const std::string & view, std::map<std::string, std::string> & map, char from, char to) {

	std::string copy(view);


	size_t o = 0;
	unsigned status = m2::TemplateEngine::NoneIf;
	std::string key;
	std::pair<size_t, size_t> del;

	while (o != std::string::npos) {
		o = copy.find(from, o);
		if (o == std::string::npos || (o + 1) == copy.size()) break;
		size_t c = copy.find(to, o + 1);
		if (c == std::string::npos) break;

		key = copy.substr(o + 1, c - (o + 1));

		if (status == m2::TemplateEngine::NoneIf) { //if we are not already in an if-block
			if (copy[o + 1] == '#') { // check if tag is an opening if-tag indicated by '#'
				status = m2::TemplateEngine::OpenIf;
				key = copy.substr(o + 2, c - (o + 2));
				if (map.find(key) == map.end()) { // if key not found we must delete the whole if-block
					status |= m2::TemplateEngine::IsFalseIf;
					del.first = o; // save del position for later
					o = c + 1; // move one position after opening if-tag for next iteration
				}
				else {
					status |= m2::TemplateEngine::IsTrueIf;
					copy.replace(o, (c + 1) - o, ""); // if TRUE remove opening if-tag
				}
				continue; // Opening if-tag was processed go to next iteration
			}


		}
		else if (status & m2::TemplateEngine::OpenIf) {
			if (copy[o + 1] == '/') {
				if (status & m2::TemplateEngine::IsTrueIf) {
					copy.replace(o, (c + 1) - o, ""); // if TRUE remove closing if-tag
				}
				else if (status & m2::TemplateEngine::IsFalseIf) {
					del.second = c + 1;
					o = del.first - 1;
					copy.replace(del.first, del.second - del.first, ""); // if FALSE remove whole block

				}
				status = m2::TemplateEngine::NoneIf;
				continue;
			}
		}

		std::string word = map[key];

		copy.replace(o, (c + 1) - o, word);



	}

	return copy;

}
