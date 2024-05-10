#!/usr/bin/env python3

import subprocess
import os
from shutil import copy2
import sys


def run_command(command):
    """Run a shell command and return the output."""
    try:
        output = subprocess.check_output(
            command, stderr=subprocess.STDOUT, shell=True, text=True
        )
        return output.strip()
    except subprocess.CalledProcessError as e:
        print(f"Error executing {command}: {e.output}")
        return None


def fix_library(lib_path, target_dir):
    lib_basename = os.path.basename(lib_path)

    # Copy the library if it's not already in the target directory
    target_lib_path = os.path.join(target_dir, lib_basename)
    if not os.path.isfile(target_lib_path):
        copy2(lib_path, target_dir)
        print(f"Copied {lib_basename} to {target_dir}")

    first_path = (
        run_command(f"otool -L {lib_path} | awk '{{print $1}}' | head -n 1")
        .strip()
        .replace(":", "")
    )
    # Update the install name of the library
    new_install_name = f"@executable_path/../Frameworks/{lib_basename}"
    run_command(f"install_name_tool -id {new_install_name} {target_lib_path}")
    print(f"install_name_tool -id {new_install_name} {target_lib_path}")
    print(f"Updated install name for {lib_basename}")
    # Get the library's dependencies
    dependencies = run_command(f"otool -L {lib_path}").splitlines()[2::]
    print(first_path)
    if dependencies:
        for line in dependencies:
            if "/opt/homebrew/" in line or "@rpath" in line:
                dep_path = line.split()[0]
                print(dep_path)
                dep_basename = os.path.basename(dep_path)

                # Resolve @rpath
                if "@rpath" in dep_path:
                    rpath = dep_path
                    actual_path = dep_path.replace("@rpath", os.path.dirname(lib_path))
                    dep_path = actual_path
                    print(dep_path, "dep_path")

                    # Change the dependency path in the current library
                    if os.path.exists(dep_path):
                        run_command(
                            f"install_name_tool -change {rpath} '@executable_path/../Frameworks/{dep_basename}' {target_lib_path}"
                        )
                        print(
                            f"install_name_tool -change {rpath} '@executable_path/../Frameworks/{dep_basename}' {target_lib_path}"
                        )

                        # Recursively fix dependencies
                        fix_library(dep_path, target_dir)
                    else:
                        print(f"Dependency file {dep_path} not found, skipping...")

                else:
                    if os.path.exists(dep_path):
                        run_command(
                            f"install_name_tool -change {dep_path} '@executable_path/../Frameworks/{dep_basename}' {target_lib_path}"
                        )
                        print(
                            f"install_name_tool -change {dep_path} '@executable_path/../Frameworks/{dep_basename}' {target_lib_path}"
                        )

                        # Recursively fix dependencies
                        fix_library(dep_path, target_dir)
                    else:
                        print(f"Dependency file {dep_path} not found, skipping...")


def fix_executable(exec_path, frameworks_dir):
    # Update the install name of the executable
    exe_basename = os.path.basename(exec_path)
    # Get the executable's dependencies
    dependencies = run_command(f"otool -L {exec_path}")
    if dependencies:
        for line in dependencies.splitlines():
            if "/opt/homebrew/" in line or "@rpath" in line:
                dep_path = line.split()[0]
                dep_basename = os.path.basename(dep_path)

                # Change the dependency path in the executable
                new_path = f"@executable_path/../Frameworks/{dep_basename}"
                run_command(
                    f"install_name_tool -change {dep_path} {new_path} {exec_path}"
                )
                print(
                    f"Changed dependency path in {exe_basename} to use local frameworks directory"
                )


app_exec = sys.argv[2] + "/scrcpy-macos"
frameworks_dir = sys.argv[1] + "/Frameworks"

dependencies = run_command(f"otool -L {app_exec}").splitlines()[1::]
for dep in dependencies:
    if "/opt/homebrew/" in dep or "@rpath" in dep:
        dep_path = dep.split()[0]
        run_command(
            f"install_name_tool -change {dep_path} '@executable_path/../Frameworks/{os.path.basename(dep_path)}' {app_exec}"
        )
        fix_library(dep_path, frameworks_dir)
