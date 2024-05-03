#include "PC_1.hpp"
bool areAllPathCharactersValid(const std::string &path)
{
	const std::string allowedSpecialChars = "_-.~:/?#[]@!$&'()*+,;=";
	for (size_t i = 0; i < path.length(); i++)
	{
		if (!isalnum(path[i]) && allowedSpecialChars.find(path[i]) == std::string::npos)
			return false;
	}
	return true;
}

bool isSafePath(const std::string &path)
{
	// Check for directory traversal
	return path.find("..") == std::string::npos;
}

bool isPathLengthValid(const std::string &path, size_t maxLength)
{
	return path.length() <= maxLength;
}

//----------------------------------------------------------------------------------------------------------

void printMap(const std::map<std::string, std::string>& post_file_map) {
    // Vérification si la map est vide
    if (post_file_map.empty()) {
        std::cout << "The map is empty." << std::endl;
        return;
    }

    // Déclaration d'un itérateur pour parcourir la map
    std::map<std::string, std::string>::const_iterator it;

    // Parcours de la map avec l'itérateur
    for (it = post_file_map.begin(); it != post_file_map.end(); ++it) {
        std::cout << "Key: " << it->first << " - Value: " << it->second << std::endl;
    }
}

void Part_C::parse(const std::string& requestText, s_server2& config)
{
    std::istringstream requestStream(requestText);
    std::string line;

    // Parse de la ligne de requête
    if (std::getline(requestStream, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> uri >> httpVersion;
    }
    if (method != "GET" && method != "POST" && method != "DELETE")
	{
		status = 501;
		throw Part_C::InvalidRequestException("Error Method 501");
	}
    if (!areAllPathCharactersValid(uri))
	{
		status = 400;
		throw Part_C::InvalidRequestException("Error Uri 400");
	}
	if (!isSafePath(uri))
	{
		status = 403;
		throw Part_C::InvalidRequestException("Error Uri 403");
	}
	if (!isPathLengthValid(uri, 4096))
	{
		status = 414;
		throw Part_C::InvalidRequestException("Error Uri 414");
	}
    if (httpVersion != "HTTP/1.1" && httpVersion != "HTTP/1.0")
	{
		status = 505;
		throw Part_C::InvalidRequestException("Error HttpVersion 505");
	}

    // Parse des en-têtes
    while (std::getline(requestStream, line))
    {
        if (line.empty() || line[0] == '\r') break; // Vérifie la fin des en-têtes grâce à la ligne vide
        std::string::size_type colonPos = line.find(":");
        if (colonPos != std::string::npos && colonPos != 0)
        { // Vérifie aussi que ':' n'est pas le premier caractère
            std::string headerName = line.substr(0, colonPos);
            // Assurez-vous qu'il y a un espace après ':' avant de commencer la valeur
            std::string headerValue = (colonPos + 2 < line.size()) ? line.substr(colonPos + 2) : "";

            // Trim leading and trailing whitespaces from headerName and headerValue if necessary
            // ...

            if (!headerName.empty() && !headerValue.empty())
            { // Vérifie que ni le nom ni la valeur de l'en-tête ne sont vides
                headers[headerName] = headerValue; // Ajout de l'en-tête à la map
            }
            else
            {
                // Gestion d'erreur ou log si le nom de l'en-tête ou la valeur est vide
                status = 400;
		        throw Part_C::InvalidRequestException("Error Headers 400");
            }
        }
        else
        {
            // Gestion d'erreur ou log si la ligne d'en-tête est mal formée (pas de ':' ou ':' est le premier caractère)
            status = 400;
            std::cout << "\nTEST ERROR\n";
		    throw Part_C::InvalidRequestException("Error Headers 400");
        }
    }

    // Le reste est le corps de la requête
    if(method == "DELETE")
        return;


    std::istreambuf_iterator<char> begin(requestStream), end;
    std::string potential_body(begin, end);

    if (potential_body.size() > config.client_max_body_size)
    {
        status = 413;
        throw Part_C::InvalidRequestException("Error Body 413");
    }
    if (headers.find("Content-Type") != headers.end())
    {
        if(headers["Content-Type"].find("multipart/form-data") != std::string::npos)
        {
            std::cout << "\n-----> Body form multipart/form-data\n";
            /*std::map<std::string, std::string> post_file_map = */parseMultiPartBody(potential_body);
            /*printMap(post_file_map);
            post_file_name = post_file_map["filename"];
            post_file_content = post_file_map["content"];*/
        }
        else if(headers["Content-Type"].find("application/x-www-form-urlencoded") != std::string::npos)
        {
            std::cout << "\n-----> Body form application/x-www-form-urlencoded\n" << potential_body << std::endl;
            std::map<std::string, std::string> post_file_map = parseUrlEncoded(potential_body);
            printMap(post_file_map);
            post_file_name = post_file_map["filename"];
            post_file_content = post_file_map["content"];
        }
        else
        {
            std::cout << "\n-----> Body form not supported\n\n";
            status = 415;
            throw Part_C::InvalidRequestException("Error Body 415");
        }
    }
    std::cout << "post_file_name : " << post_file_content << "\n";
    std::string body = potential_body;
}

void Part_C::print_parse()
{
    std::cout << "-------------------> Impression du parsing\n";
    std::cout << "method : " << method << std::endl;
    std::cout << "uri : " << uri << std::endl;
    std::cout << "httpVersion : " << httpVersion << std::endl;
    std::cout << "headers : " << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::cout << "     " << it->first << " : " << it->second << std::endl;
    }
    std::cout << "body : " << body << std::endl;
    std::cout << std::endl;
}

//----------------------------------------------------------------------------------------------------------

std::string Part_C::getMultiPartBoundary()
{
	std::string content_type = headers.find("Content-Type")->second;
	std::string boundaryPrefix = "boundary=";
    size_t boundaryPos = content_type.find(boundaryPrefix);
    
    if (boundaryPos == std::string::npos) {
        throw std::runtime_error("Boundary delimiter not found in Content-Type");
    }

    boundaryPos += boundaryPrefix.length();
    size_t boundaryEnd = content_type.find(";", boundaryPos);
    
    if (boundaryEnd == std::string::npos) {
        // Boundary is at the end of the string
        return content_type.substr(boundaryPos);
    } else {
        // Boundary ends before a semicolon
        return content_type.substr(boundaryPos, boundaryEnd - boundaryPos);
    }
}

void Part_C::parseMultiPartBody(std::string bodyLines)
{
    //std::cout << LOG_COLOR << "[LOG] " << RESET << "Content-Type: multipart/form-data" << std::endl;
	std::string boundary = getMultiPartBoundary();
    //std::string test = "123456789";

	std::size_t start = bodyLines.find(boundary);
	if (start == std::string::npos)
	{
		status = 400; // Bad Request
		throw Part_C::InvalidRequestException("Error Parse Multi Part Body 400");
	}
    //std::cout << test + "XX\n";

    boundary[boundary.length() - 1] = '-';
    boundary[boundary.length()] = '-';
    boundary[boundary.length() + 1] = '\0';
    std::cout << boundary + "\n";

	std::size_t end = bodyLines.find(boundary);
	if (end == std::string::npos)
	{
        std::cout << "no end boundary\n";
		status = 400; // Bad Request
		throw Part_C::InvalidRequestException("Error Parse Multi Part Body 400");
	}
	std::string headers_and_body = bodyLines.substr(start + boundary.length(), end - start - boundary.length());
	std::string headers = headers_and_body.substr(0, headers_and_body.find(std::string("\r\n") + std::string("\r\n")));
	std::string body = headers_and_body.substr(headers_and_body.find(std::string("\r\n") + std::string("\r\n")) + 4);
	std::size_t filename_start = headers.find("name=");
	if (filename_start == std::string::npos)
	{
        std::cout << "other\n";
		status = 400; // Bad Request
		throw Part_C::InvalidRequestException("Error Parse Multi Part Body 400");
	}
	std::size_t filename_end = headers.find('"', filename_start + 6);
	post_file_name = headers.substr(filename_start + 6, filename_end - filename_start - 6);

	post_file_content = body.substr(0, body.length() - 2);
}

std::map<std::string, std::string> Part_C::parseUrlEncoded(const std::string& data)
{
    std::map<std::string, std::string> result;
    std::istringstream dataStream(data);
    std::string pair;

    while (std::getline(dataStream, pair, '&'))
    {
        std::string::size_type delimiterPos = pair.find('=');
        if (delimiterPos != std::string::npos)
        {
            std::string key = pair.substr(0, delimiterPos);
            std::string value = pair.substr(delimiterPos + 1);

            // Remplacer '+' par ' ' dans value
            std::replace(value.begin(), value.end(), '+', ' ');

            // Décoder les caractères en pourcentage ici si nécessaire

            result[key] = value;
        }
    }

    return result;
}

std::map<std::string, std::string> parseUrlEncodedData(const std::string& data)
{
    std::map<std::string, std::string> result;
    std::istringstream dataStream(data);
    std::string pair;

    while (std::getline(dataStream, pair, '&'))
    {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos)
        {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            result[key] = value; // Dans une version complète, vous devriez décoder les pourcentages ici.
        }
    }

    return result;
}