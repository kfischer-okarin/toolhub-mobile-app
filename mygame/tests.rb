def require_test_files(parent_folder = nil)
  parent_folder ||= 'tests'
  $gtk.list_files(parent_folder).each do |file|
    next if file == 'test_helper.rb'

    path = "#{parent_folder}/#{file}"
    if file.end_with?('.rb')
      require_relative path
    elsif $gtk.stat_file(path)[:file_type] == :directory
      require_test_files(path)
    end
  end
end

require_test_files
