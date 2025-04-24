#!/usr/bin/env python3

import os
import sys
import urllib

class SaveForm:
	def __init__( self, fields:dict[str,str] ):
		self.fields = fields

	def get_form_to_write( self ) -> str:
		form_info = ""

		for key, value in self.fields.items():
			# print("key:", key, "value:", value, file=sys.stderr)
			form_info += key + " = " + urllib.parse.unquote(value[:500])
			if form_info[-1] != '\n':
				form_info += "; "
		form_info += "\n"
		return form_info

	def store_form_information( self ) -> None:
		path = "data/uploads/contact_info.txt"
		try:
			with open(path, "a") as file:
				file.write(self.get_form_to_write())
		except Exception as e:
			print(f"Error writing to form file: {e}", file=sys.stderr)

