"use client"

import { useEffect, useState } from "react"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardHeader } from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Table, TableBody, TableCell, TableHead, TableHeader, TableRow } from "@/components/ui/table"
import { Search, UserCog, Loader2 } from "lucide-react"
import Link from "next/link"
import { CreateUserDialog } from "@/components/create-user-dialog"
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from "@/components/ui/select"
import Cookies from "js-cookie"

interface Operator {
  id: number
  name: string
  fingerprint_id: number
  role: string
  email?: string
  phone?: string
  status: string
  created_at: string
}

export default function OperatorsPage() {
  const [operators, setOperators] = useState<Operator[]>([])
  const [loading, setLoading] = useState(true)
  const [searchTerm, setSearchTerm] = useState("")
  const [statusFilter, setStatusFilter] = useState("")

  useEffect(() => {
    fetchOperators()
  }, [searchTerm, statusFilter])

  const fetchOperators = async () => {
    try {
      const token = Cookies.get('access_token')
      const params = new URLSearchParams()
      
      if (searchTerm) params.append('search', searchTerm)
      if (statusFilter) params.append('status_filter', statusFilter)
      
      const response = await fetch(`http://localhost:8000/operators/?${params}`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      })

      if (response.ok) {
        const data = await response.json()
        setOperators(data)
      } else {
        console.error('Failed to fetch operators')
      }
    } catch (error) {
      console.error('Error fetching operators:', error)
    } finally {
      setLoading(false)
    }
  }

  const handleSearch = (value: string) => {
    setSearchTerm(value)
  }

  const handleStatusFilter = (value: string) => {
    setStatusFilter(value === "all" ? "" : value)
  }

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <Loader2 className="h-8 w-8 animate-spin" />
      </div>
    )
  }

  return (
    <div className="flex flex-col gap-4">
      <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
        <div>
          <h1 className="text-2xl md:text-3xl font-bold tracking-tight">Operators</h1>
          <p className="text-muted-foreground">Manage operator accounts and biometric access</p>
        </div>
        <CreateUserDialog onUserCreated={fetchOperators} />
      </div>

      <Card>
        <CardHeader className="p-4">
          <div className="flex flex-col sm:flex-row sm:items-center gap-4">
            <div className="flex items-center gap-2 w-full max-w-sm">
              <Search className="h-4 w-4 text-muted-foreground" />
              <Input 
                placeholder="Search operators..." 
                className="h-9"
                value={searchTerm}
                onChange={(e) => handleSearch(e.target.value)}
              />
            </div>
            <div className="flex items-center gap-2 ml-auto">
              <Select value={statusFilter || "all"} onValueChange={handleStatusFilter}>
                <SelectTrigger className="w-32">
                  <SelectValue placeholder="Status" />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="all">All Status</SelectItem>
                  <SelectItem value="Active">Active</SelectItem>
                  <SelectItem value="Pending">Pending</SelectItem>
                  <SelectItem value="Suspended">Suspended</SelectItem>
                </SelectContent>
              </Select>
              <Button variant="outline" size="sm">
                Export
              </Button>
            </div>
          </div>
        </CardHeader>
        <CardContent className="p-0 overflow-auto">
          <div className="w-full min-w-[640px]">
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>Name</TableHead>
                  <TableHead>Fingerprint ID</TableHead>
                  <TableHead>Email</TableHead>
                  <TableHead>Phone</TableHead>
                  <TableHead>Role</TableHead>
                  <TableHead>Status</TableHead>
                  <TableHead className="text-right">Actions</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {operators.length === 0 ? (
                  <TableRow>
                    <TableCell colSpan={7} className="text-center py-8 text-muted-foreground">
                      No operators found
                    </TableCell>
                  </TableRow>
                ) : (
                  operators.map((operator) => (
                    <TableRow key={operator.id}>
                      <TableCell className="font-medium">
                        {operator.name}
                      </TableCell>
                      <TableCell>{operator.fingerprint_id}</TableCell>
                      <TableCell>{operator.email || '-'}</TableCell>
                      <TableCell>{operator.phone || '-'}</TableCell>
                      <TableCell>
                        <span className={`inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-semibold ${
                          operator.role === "admin"
                            ? "bg-purple-50 text-purple-700 dark:bg-purple-900/20 dark:text-purple-300"
                            : "bg-blue-50 text-blue-700 dark:bg-blue-900/20 dark:text-blue-300"
                        }`}>
                          {operator.role}
                        </span>
                      </TableCell>
                      <TableCell>
                        <div
                          className={`inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-semibold ${
                            operator.status === "Active"
                              ? "bg-green-50 text-green-700 dark:bg-green-900/20 dark:text-green-300"
                              : operator.status === "Pending"
                              ? "bg-yellow-50 text-yellow-700 dark:bg-yellow-900/20 dark:text-yellow-300"
                              : "bg-red-50 text-red-700 dark:bg-red-900/20 dark:text-red-300"
                          }`}
                        >
                          {operator.status}
                        </div>
                      </TableCell>
                      <TableCell className="text-right">
                        <Button variant="ghost" size="icon" asChild>
                          <Link href={`/users/${operator.id}`}>
                            <UserCog className="h-4 w-4" />
                            <span className="sr-only">Edit operator</span>
                          </Link>
                        </Button>
                      </TableCell>
                    </TableRow>
                  ))
                )}
              </TableBody>
            </Table>
          </div>
        </CardContent>
      </Card>
    </div>
  )
}
