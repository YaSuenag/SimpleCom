/*
 * Copyright (C) 2021, Yasumasa Suenaga
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
using System;
using System.Collections;
using System.ComponentModel;
using System.Configuration.Install;
using System.IO;

namespace SimpleComInstaller
{
    [RunInstaller(true)]
    public class CustomActions : Installer
    {

        private const string fragment_json = @"
            {
                ""profiles"": [
                    {
                        ""name"": ""SimpleCom"",
                        ""commandline"": ""{PATH}""
                    }
                ]
            }
        ";

        private string FragmentPath => Environment.GetEnvironmentVariable(this.Context.Parameters["allusers"] == "1" ? "ProgramData" : "LocalAppData")
                                            + @"\Microsoft\Windows Terminal\Fragments\SimpleCom";

        public override void Install(IDictionary stateSaver)
        {
            base.Install(stateSaver);
            var path = this.Context.Parameters["TargetDir"];
            var fragments = fragment_json.Replace("{PATH}", (path.Trim() + @"SimpleCom.exe").Replace(@"\", @"\\"));

            if (!Directory.Exists(FragmentPath))
            {
                Directory.CreateDirectory(FragmentPath);
            }
            File.WriteAllText(FragmentPath + @"\fragments.json", fragments);
        }

        public override void Rollback(IDictionary savedState)
        {
            base.Rollback(savedState);
            Directory.Delete(FragmentPath, true);
        }

        public override void Uninstall(IDictionary savedState)
        {
            base.Uninstall(savedState);
            Directory.Delete(FragmentPath, true);
        }
    }
}
