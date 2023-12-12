#
#  OSLG  -- Operating System Loader in GRUB 
#  Copyright (C) 2022 Free Software Foundation, Inc.
#
#  OSLG is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OSLG is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with OSLG.  If not, see <http://www.gnu.org/licenses/>.
#

import locale
import sys

def get_system_language():
    system_locale, _ = locale.getdefaultlocale()
    system_language = system_locale.split('_')[0] if system_locale else 'en'
    return system_language
    
def tr(phrase):
        system_language = get_system_language()
        translations = {
            'es': {'Generating configuration file': 'Generando fichero de configuracion'
            ,'Found' : 'Encontrado'
            ,'Found linux image' : 'Encontrada imagen de linux'
            ,'Found intrd image' : 'Encontrada imagen de memoria inicial' 
            ,'on' : 'en' 
            ,'The *{platform}-oslg-probe command was not found on the system.' : 'No se encontro el comando *{platform}-oslg-probe en el sistema.'
            ,f'The program can only be used in root mode, use sudo {sys.argv[0]} to continue' : f'El programa solo puede usarse en modo root, usa sudo {sys.argv[0]} para continuar'
            ,'Theme used by default:' : 'Thema usado por defecto:'
            ,'Generate OSLG configuration file.' : 'Genera un fichero de configuración OSLG.'
            ,'Output of the selected file.' : 'Salida del fichero seleccionado.' 
            ,'File not selected, use -o {path}' : 'Fichero no seleccionado, usa -o {path}'
            ,'Boot directory.' : 'Directorio de arranque.' 
            ,'Set the order of the input menu.' : 'Colocar el orden del menu de entrada.'
            ,'No boot directory selected, use -r or --root' : 'No se seleccionó ningún directorio de inicio, use -r o --root'
            ,'Set timeout of the input menu.' : 'Colocar el tiempo del menu de entrada.'
            ,'Boot command cmdline boot os entry linux.' : 'Comando de arranque cmdline boot os entrada linux.'
            ,'Disable Linux Recovery from startup.' : 'Deshabilitar la recuperación de Linux desde el inicio.'
            ,'Set the input menu screen size.' : 'Establezca el tamaño de pantalla del menu de entrada.'
            ,'Invalid Sintax gfxmode' : 'Sintaxis gfxmode no válida'
            ,'The *{platform}-oslg-devicemap command was not found on the system.' : 'No se encontro el comando *{platform}-oslg-devicemap en el sistema.'
            ,'Set theme directory.' : 'Colocar directorio de temas.'},

            'en': {'Generando fichero de configuracion' : 'Generating configuration file'
            ,'Encontrado' : 'Found'
            ,'Encontrada imagen de linux' : 'Found linux image'
            ,'Encontrada imagen de memoria inicial' : 'Found intrd image'
            ,'en' : 'on'
            ,'No se encontro el comando *{platform}-oslg-probe en el sistema.' : 'The *{platform}-oslg-probe command was not found on the system.' 
            ,f'El programa solo puede usarse en modo root, usa sudo {sys.argv[0]} para continuar' : f'The program can only be used in root mode, use sudo {sys.argv[0]} to continue' 
            ,'Thema usado por defecto:' : 'Theme used by default:'
            ,'Genera un fichero de configuración OSLG.' : 'Generate OSLG configuration file.'
            ,'Salida del fichero seleccionado.' : 'Output of the selected file.'
            ,'Fichero no seleccionado, usa -o {path}' : 'File not selected, use -o {path}'
            ,'Directorio de arranque.' : 'Boot directory.'
            ,'Colocar el orden del menu de entrada.' : 'Set the order of the input menu.'
            ,'No se seleccionó ningún directorio de inicio, use -r o --root' : 'No boot directory selected, use -r or --root'
            ,'Colocar el tiempo del menu de entrada.' : 'Set timeout of the input menu.'
            ,'Comando de arranque cmdline boot os entrada linux.' : 'Boot command cmdline boot os entry linux.'
            ,'Deshabilitar la recuperación de Linux desde el inicio.' : 'Disable Linux Recovery from startup.'
            ,'Establezca el tamaño de pantalla del menu de entrada.' : 'Set the input menu screen size.'
            ,'Sintaxis gfxmode no válida' : 'Invalid Sintax gfxmode'
            , 'No se encontro el comando *{platform}-oslg-devicemap en el sistema.' : 'The *{platform}-oslg-devicemap command was not found on the system.'
            ,'Colocar directorio de temas.' : 'Set theme directory.'},
        }
        translated_phrase = translations.get(system_language, {}).get(phrase, phrase)
        return translated_phrase

